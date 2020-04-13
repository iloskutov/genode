/*
 * \brief  EHCI for a64
 * \author
 * \date   2020-03-29
 *
 * TODO: clean up
 */
#include <base/attached_io_mem_dataspace.h>
#include <io_mem_session/connection.h>
#include <util/mmio.h>

#include <platform.h>
#include <lx_emul.h>

extern "C" void module_ehci_hcd_init();
extern "C" void module_ohci_hcd_mod_init();

extern "C" int ehci_platform_init(void); // TODO
extern "C" int ohci_platform_init(void);

using namespace Genode;

// drivers/phy/allwinner/phy-sun4i-usb.c
struct Pmu : Genode::Mmio
{
	struct PmuR  : Register<0, 32> { };

#define REG_PMU_UNK1			0x10
	struct Unk1  : Register<REG_PMU_UNK1, 32> { };

	unsigned index;

	Pmu(Genode::Env & env, unsigned i, addr_t const base)
	: Mmio(base), index(i)
	{
		Genode::log("USB PHY PMU init...\n");
	}
};

struct Phy_usb : Genode::Mmio
{
	Attached_io_mem_dataspace io_pmu0, io_pmu1;
	Pmu pmu0, pmu1;

#define PHY_TX_AMPLITUDE_TUNE		0x20
#define PHY_DISCON_TH_SEL		0x2a
#define PHYCTL_DATA			BIT(7)

	struct Ctrl : Register<0x10, 32> { };
	struct Ctrlb : Register<0x10, 8> { };


	void sun4i_usb_phy_write(Pmu &pmu, uint32_t addr, uint32_t data, int len)
	{
		uint32_t usbc_bit = BIT(pmu.index * 2);

		write<Phy_usb::Ctrl>(0);

		for (int i = 0; i < len; i++) {
			uint32_t temp = read<Phy_usb::Ctrl>();

			/* clear the address portion */
			temp &= ~(0xff << 8);

			/* set the address */
			temp |= ((addr + i) << 8);
			write<Phy_usb::Ctrl>(temp);

			/* set the data bit and clear usbc bit*/
			temp = read<Phy_usb::Ctrlb>();
			if (data & 0x1)
				temp |= PHYCTL_DATA;
			else
				temp &= ~PHYCTL_DATA;
			temp &= ~usbc_bit;
			write<Phy_usb::Ctrlb>(temp);

			/* pulse usbc_bit */
			temp = read<Phy_usb::Ctrlb>();
			temp |= usbc_bit;
			write<Phy_usb::Ctrlb>(temp);

			temp = read<Phy_usb::Ctrlb>();
			temp &= ~usbc_bit;
			write<Phy_usb::Ctrlb>(temp);

			data >>= 1;
		}

	}

	void sun4i_usb_phy_passby(Pmu &pmu, int enable)
	{
#define SUNXI_AHB_ICHR8_EN		BIT(10)
#define SUNXI_AHB_INCR4_BURST_EN	BIT(9)
#define SUNXI_AHB_INCRX_ALIGN_EN	BIT(8)
#define SUNXI_ULPI_BYPASS_EN		BIT(0)

		uint32_t bits, reg_value;
		bits = SUNXI_AHB_ICHR8_EN | SUNXI_AHB_INCR4_BURST_EN |
				SUNXI_AHB_INCRX_ALIGN_EN | SUNXI_ULPI_BYPASS_EN;

		reg_value = pmu.read<Pmu::PmuR>();

		if (enable)
			reg_value |= bits;
		else
			reg_value &= ~bits;

		pmu.write<Pmu::PmuR>(reg_value);
	}

	Phy_usb(Genode::Env & env, addr_t const base)
	: Mmio(base),
	  io_pmu0(env, 0x01c1a800, 0x14), // ??? 0x4
	  io_pmu1(env, 0x01c1b800, 0x14),
	  pmu0(env, 0, (addr_t)io_pmu0.local_addr<addr_t>()),
	  pmu1(env, 1, (addr_t)io_pmu1.local_addr<addr_t>())
	{
		Genode::log("USB PHY init...\n");

		#define REG_PHYCTL_OFF			0x10

		uint32_t val = pmu1.read<Pmu::Unk1>();
		val &= ~2;
		pmu1.write<Pmu::Unk1>(val);


		sun4i_usb_phy_write(pmu1, PHY_TX_AMPLITUDE_TUNE, 0x14, 5);

		sun4i_usb_phy_write(pmu1, PHY_DISCON_TH_SEL,
				    /* data->cfg->disc_thresh */ 0x3, 2);

		sun4i_usb_phy_passby(pmu1, 1);

	}
};

struct Clk_usb : Genode::Mmio
{
	struct PllHsic : Register<0x44, 32> {};

	struct BusClkGating0 : Register<0x60, 32> {
		struct Otg : Bitfield<23, 1> { };
		struct Ehci0 : Bitfield<24, 1> { };
		struct Ehci1 : Bitfield<25, 1> { };
		struct Ohci0 : Bitfield<28, 1> { };
		struct Ohci1 : Bitfield<29, 1> { };
	};

	struct UsbphyCfg : Register<0xcc, 32> {
		struct Phy0Rst : Bitfield<0, 1> { };
		struct Phy1Rst : Bitfield<1, 1> { };
		struct HsicRst : Bitfield<2, 1> { };

		struct Phy0 : Bitfield<8, 1> { };
		struct Phy1 : Bitfield<9, 1> { };
		struct Hsic : Bitfield<10, 1> { };
		struct Hsic12M : Bitfield<11, 1> { };

		struct SclkGatingOhci : Bitfield<16, 2> { };
	};

	struct BusSoftRst0 : Register<0x2c0, 32> {
		struct Ehci0 : Bitfield<24, 1> { };
		struct Ehci1 : Bitfield<25, 1> { };
		struct Ohci0 : Bitfield<28, 1> { };
		struct Ohci1 : Bitfield<29, 1> { };
	};


	Clk_usb(Genode::Env & env, addr_t const base) : Mmio(base)
	{
		Genode::log("USB clk init...\n");

		uint32_t r = Mmio::read<PllHsic>();
		Genode::log("PLLHSIC: ", Genode::Hex(r));
		r = Mmio::read<BusClkGating0>();
		Genode::log("BusClkGating0: ", Genode::Hex(r));
		r = Mmio::read<UsbphyCfg>();
		Genode::log("UsbphyCfg: ", Genode::Hex(r));
		r = Mmio::read<BusSoftRst0>();
		Genode::log("BusSoftRst0: ", Genode::Hex(r));

		UsbphyCfg::access_t  usbphycfg = 0;//Mmio::read<UsbphyCfg>();
		// UsbphyCfg::Phy0Rst::set(usbphycfg, 0);
		// UsbphyCfg::Phy1Rst::set(usbphycfg, 0);
		// UsbphyCfg::HsicRst::set(usbphycfg, 0);
        Mmio::write<UsbphyCfg>(usbphycfg);

		mdelay(10);

		UsbphyCfg::Phy0Rst::set(usbphycfg, 1);
		UsbphyCfg::Phy1Rst::set(usbphycfg, 1);
		UsbphyCfg::HsicRst::set(usbphycfg, 1);
        Mmio::write<UsbphyCfg>(usbphycfg);

		UsbphyCfg::Phy0::set(usbphycfg, 1);
		UsbphyCfg::Phy1::set(usbphycfg, 1);
		UsbphyCfg::Hsic::set(usbphycfg, 1);
		UsbphyCfg::Hsic12M::set(usbphycfg, 1);
		UsbphyCfg::SclkGatingOhci::set(usbphycfg, 3);
        Mmio::write<UsbphyCfg>(usbphycfg);

		BusClkGating0::access_t  clk = Mmio::read<BusClkGating0>();
		BusClkGating0::Ehci0::set(clk, 1);
		BusClkGating0::Ehci1::set(clk, 1);
		BusClkGating0::Ohci0::set(clk, 1);
		BusClkGating0::Ohci1::set(clk, 1);
        Mmio::write<BusClkGating0>(clk);

		BusSoftRst0::access_t  rst = Mmio::read<BusSoftRst0>();
		BusSoftRst0::Ehci0::set(rst, 1);
		BusSoftRst0::Ehci1::set(rst, 1);
		BusSoftRst0::Ohci0::set(rst, 1);
		BusSoftRst0::Ohci1::set(rst, 1);
        Mmio::write<BusSoftRst0>(rst);


		r = Mmio::read<BusClkGating0>();
		Genode::log("BusClkGating0: ", Genode::Hex(r));
		r = Mmio::read<UsbphyCfg>();
		Genode::log("UsbphyCfg: ", Genode::Hex(r));
		r = Mmio::read<BusSoftRst0>();
		Genode::log("BusSoftRst0: ", Genode::Hex(r));

	}
};

void platform_hcd_init(Genode::Env &env, Services *services)
{
	/* setup clk */
	Attached_io_mem_dataspace io_clk(env, 0x01c20000, 0x400);
	Clk_usb clk(env, (addr_t)io_clk.local_addr<addr_t>());

	module_ehci_hcd_init();
	module_ohci_hcd_mod_init();

	ehci_platform_init();
	ohci_platform_init();

	/* setup PHY */
	Attached_io_mem_dataspace io_phy(env, 0x01c19400, 0x14);
	Phy_usb phy(env, (addr_t)io_phy.local_addr<addr_t>());

	/* setup EHCI-controller platform device */
	{
		static resource ehci_res[] =
		{
			{ 0x01c1b000, 0x01c1b0ff, "ehci-platform", IORESOURCE_MEM },
			{ 74 + 32, 74 + 32, "ehci-platform-irq", IORESOURCE_IRQ },
		};

		platform_device *pdev = (platform_device *)kzalloc(sizeof(platform_device), 0);
		pdev->name = (char *)"ehci-platform";
		pdev->id   = 0;
		pdev->num_resources = 2;
		pdev->resource = ehci_res;

		pdev->dev.of_node             = (device_node*)kzalloc(sizeof(device_node), 0);
		pdev->dev.of_node->properties = (property*)kzalloc(sizeof(property), 0);
		pdev->dev.of_node->properties->name  = "compatible";
		pdev->dev.of_node->properties->value = (void*)"allwinner,sun50i-a64-ehci";

		/*
		 * Needed for DMA buffer allocation. See 'hcd_buffer_alloc' in 'buffer.c'
		 */
		static u64 dma_mask = ~(u64)0;
		pdev->dev.dma_mask = &dma_mask;
		pdev->dev.coherent_dma_mask = ~0;

		platform_device_register(pdev);
	}

	/* setup OHCI-controller platform device */
	{
		static resource ohci_res[] =
		{
			{ 0x01c1b400, 0x01c1b4ff, "ohci-platform", IORESOURCE_MEM },
			{ 75 + 32, 75 + 32, "ohci-platform-irq", IORESOURCE_IRQ },
		};

		platform_device *pdev = (platform_device *)kzalloc(sizeof(platform_device), 0);
		pdev->name = (char *)"ohci-platform";
		pdev->id   = 0;
		pdev->num_resources = 2;
		pdev->resource = ohci_res;

		pdev->dev.of_node             = (device_node*)kzalloc(sizeof(device_node), 0);
		pdev->dev.of_node->properties = (property*)kzalloc(sizeof(property), 0);
		pdev->dev.of_node->properties->name  = "compatible";
		pdev->dev.of_node->properties->value = (void*)"allwinner,sun50i-a64-ohci";

		/*
		 * Needed for DMA buffer allocation. See 'hcd_buffer_alloc' in 'buffer.c'
		 */
		static u64 dma_mask = ~(u64)0;
		pdev->dev.dma_mask = &dma_mask;
		pdev->dev.coherent_dma_mask = ~0;

		platform_device_register(pdev);
	}
}

