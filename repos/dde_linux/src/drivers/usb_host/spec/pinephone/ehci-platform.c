#include <lx_emul.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <../drivers/usb/host/ehci.h>

#define EHCI_MAX_CLKS 4
#define hcd_to_ehci_priv(h) ((struct ehci_platform_priv *)hcd_to_ehci(h)->priv)

struct platform_device;
struct usb_hcd;

/**
 * struct usb_ehci_pdata - platform_data for generic ehci driver
 *
 * @caps_offset:	offset of the EHCI Capability Registers to the start of
 *			the io memory region provided to the driver.
 * @has_tt:		set to 1 if TT is integrated in root hub.
 * @port_power_on:	set to 1 if the controller needs a power up after
 *			initialization.
 * @port_power_off:	set to 1 if the controller needs to be powered down
 *			after initialization.
 * @no_io_watchdog:	set to 1 if the controller does not need the I/O
 *			watchdog to run.
 * @reset_on_resume:	set to 1 if the controller needs to be reset after
 * 			a suspend / resume cycle (but can't detect that itself).
 *
 * These are general configuration options for the EHCI controller. All of
 * these options are activating more or less workarounds for some hardware.
 */
struct usb_ehci_pdata {
	int		caps_offset;
	unsigned	has_tt:1;
	unsigned	has_synopsys_hc_bug:1;
	unsigned	big_endian_desc:1;
	unsigned	big_endian_mmio:1;
	unsigned	no_io_watchdog:1;
	unsigned	reset_on_resume:1;
	unsigned	dma_mask_64:1;

	/* Turn on all power and clocks */
	int (*power_on)(struct platform_device *pdev);
	/* Turn off all power and clocks */
	void (*power_off)(struct platform_device *pdev);
	/* Turn on only VBUS suspend power and hotplug detection,
	 * turn off everything else */
	void (*power_suspend)(struct platform_device *pdev);
	int (*pre_setup)(struct usb_hcd *hcd);
};

// #define readl(addr)         (*(volatile uint32_t *)(addr))

struct ehci_driver_overrides;

extern void ehci_init_driver(struct hc_driver *drv,
					  const struct ehci_driver_overrides *over);

static int ehci_platform_probe(struct platform_device *dev);
static int ehci_platform_remove(struct platform_device *dev);
static int ehci_platform_reset(struct usb_hcd *hcd);

struct ehci_platform_priv {
	struct clk *clks[EHCI_MAX_CLKS];
	struct reset_control *rsts;
	struct phy **phys;
	int num_phys;
	bool reset_on_resume;
};

static const struct of_device_id ehci_ids[] = {
	{ .compatible = "generic-ehci", },
	{}
};
MODULE_DEVICE_TABLE(of, ehci_ids);

static const struct platform_device_id ehci_platform_table[] = {
	{ "ehci-platform", 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, ehci_platform_table);


static struct platform_driver ehci_platform_driver = {
	.id_table	= ehci_platform_table,
	.probe		= ehci_platform_probe,
	.remove		= ehci_platform_remove,
	.driver		= {
		.name	= "ehci-platform",
		.of_match_table = ehci_ids,
	}
};

static const struct ehci_driver_overrides platform_overrides __initconst = {
	.reset =		ehci_platform_reset,
	.extra_priv_size =	sizeof(struct ehci_platform_priv),
};

static struct hc_driver __read_mostly ehci_platform_hc_driver;

static int ehci_platform_reset(struct usb_hcd *hcd)
{
	struct platform_device *pdev = to_platform_device(hcd->self.controller);
	struct usb_ehci_pdata *pdata = dev_get_platdata(&pdev->dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;

	ehci->has_synopsys_hc_bug = pdata->has_synopsys_hc_bug;

	ehci->caps = hcd->regs + pdata->caps_offset;

	// TODO: check clock and phy setup
	uint32_t params = readl(&ehci->caps->hcs_params);
	if (params == 0)
		panic("USB doesn't setup correctly\n");

	retval = ehci_setup(hcd);
	if (retval)
		return retval;

	if (pdata->no_io_watchdog)
		ehci->need_io_watchdog = 0;
	return 0;
}

static int ehci_platform_power_on(struct platform_device *dev)
{
	return 0;
}

static void ehci_platform_power_off(struct platform_device *dev)
{
}

static struct usb_ehci_pdata ehci_platform_defaults = {
	.power_on =		ehci_platform_power_on,
	.power_suspend =	ehci_platform_power_off,
	.power_off =		ehci_platform_power_off,
};

static int ehci_platform_probe(struct platform_device *dev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	struct usb_ehci_pdata *pdata = dev_get_platdata(&dev->dev);
	struct ehci_platform_priv *priv;
	struct ehci_hcd *ehci;
	int err, irq, phy_num, clk = 0;

	if (usb_disabled())
		return -ENODEV;

	/*
	 * Use reasonable defaults so platforms don't have to provide these
	 * with DT probing on ARM.
	 */
	if (!pdata)
		pdata = &ehci_platform_defaults;

	err = dma_coerce_mask_and_coherent(&dev->dev,
		pdata->dma_mask_64 ? DMA_BIT_MASK(64) : DMA_BIT_MASK(32));
	if (err) {
		dev_err(&dev->dev, "Error: DMA mask configuration failed\n");
		return err;
	}

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		dev_err(&dev->dev, "no irq provided");
		return irq;
	}

	hcd = usb_create_hcd(&ehci_platform_hc_driver, &dev->dev,
			     dev_name(&dev->dev));
	if (!hcd)
		return -ENOMEM;

	platform_set_drvdata(dev, hcd);
	dev->dev.platform_data = pdata;
	priv = hcd_to_ehci_priv(hcd);
	ehci = hcd_to_ehci(hcd);

	if (pdata->big_endian_desc)
		ehci->big_endian_desc = 1;
	if (pdata->big_endian_mmio)
		ehci->big_endian_mmio = 1;
	if (pdata->has_tt)
		hcd->has_tt = 1;
	if (pdata->reset_on_resume)
		priv->reset_on_resume = true;

	if (pdata->power_on) {
		err = pdata->power_on(dev);
		if (err < 0)
			goto err_reset;
	}

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	hcd->regs = devm_ioremap_resource(&dev->dev, res_mem);
	if (IS_ERR(hcd->regs)) {
		err = PTR_ERR(hcd->regs);
		goto err_power;
	}
	hcd->rsrc_start = res_mem->start;
	hcd->rsrc_len = resource_size(res_mem);

	err = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (err)
		goto err_power;

	device_wakeup_enable(hcd->self.controller);
	device_enable_async_suspend(hcd->self.controller);
	platform_set_drvdata(dev, hcd);

	return err;

err_power:
	if (pdata->power_off)
		pdata->power_off(dev);
err_reset:
#if 0
	reset_control_assert(priv->rsts);
#endif
err_put_clks:
#if 0
	while (--clk >= 0)
		clk_put(priv->clks[clk]);
#endif
err_put_hcd:
	if (pdata == &ehci_platform_defaults)
		dev->dev.platform_data = NULL;

	usb_put_hcd(hcd);

	return err;

}

static int ehci_platform_remove(struct platform_device *dev)
{
	return 0;
}

int ehci_platform_init(void)
{
	ehci_init_driver(&ehci_platform_hc_driver, &platform_overrides);

	return platform_driver_register(&ehci_platform_driver);
}
