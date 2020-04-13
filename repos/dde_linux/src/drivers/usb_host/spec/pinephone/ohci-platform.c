// SPDX-License-Identifier: GPL-2.0
/*
 * Generic platform ohci driver
 *
 * Copyright 2007 Michael Buesch <m@bues.ch>
 * Copyright 2011-2012 Hauke Mehrtens <hauke@hauke-m.de>
 * Copyright 2014 Hans de Goede <hdegoede@redhat.com>
 *
 * Derived from the OCHI-SSB driver
 * Derived from the OHCI-PCI driver
 * Copyright 1999 Roman Weissgaerber
 * Copyright 2000-2002 David Brownell
 * Copyright 1999 Linus Torvalds
 * Copyright 1999 Gregory P. Smith
 */
#include <lx_emul.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ohci_pdriver.h>
#include <../drivers/usb/host/ohci.h>

#define DRIVER_DESC "OHCI generic platform driver"
#define OHCI_MAX_CLKS 3
#define hcd_to_ohci_priv(h) ((struct ohci_platform_priv *)hcd_to_ohci(h)->priv)

struct ohci_platform_priv {
	struct clk *clks[OHCI_MAX_CLKS];
	struct reset_control *resets;
	struct phy **phys;
	int num_phys;
};

struct usb_ohci_pdata {
	unsigned	big_endian_desc:1;
	unsigned	big_endian_mmio:1;
	unsigned	no_big_frame_no:1;
	unsigned int	num_ports;

	/* Turn on all power and clocks */
	int (*power_on)(struct platform_device *pdev);
	/* Turn off all power and clocks */
	void (*power_off)(struct platform_device *pdev);
	/* Turn on only VBUS suspend power and hotplug detection,
	 * turn off everything else */
	void (*power_suspend)(struct platform_device *pdev);
};

static const char hcd_name[] = "ohci-platform";

static int ohci_platform_power_on(struct platform_device *dev)
{
    return 0;
}

static void ohci_platform_power_off(struct platform_device *dev)
{
}

static struct hc_driver __read_mostly ohci_platform_hc_driver;

static const struct ohci_driver_overrides platform_overrides __initconst = {
	.product_desc =		"Generic Platform OHCI controller",
	.extra_priv_size =	sizeof(struct ohci_platform_priv),
};

static struct usb_ohci_pdata ohci_platform_defaults = {
	.power_on =		ohci_platform_power_on,
	.power_suspend =	ohci_platform_power_off,
	.power_off =		ohci_platform_power_off,
};

static int ohci_platform_probe(struct platform_device *dev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	struct usb_ohci_pdata *pdata = dev_get_platdata(&dev->dev);
	struct ohci_platform_priv *priv;
	struct ohci_hcd *ohci;
	int err, irq, phy_num, clk = 0;

	if (usb_disabled())
		return -ENODEV;

	/*
	 * Use reasonable defaults so platforms don't have to provide these
	 * with DT probing on ARM.
	 */
	if (!pdata)
		pdata = &ohci_platform_defaults;

	err = dma_coerce_mask_and_coherent(&dev->dev, DMA_BIT_MASK(32));
	if (err)
		return err;

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		dev_err(&dev->dev, "no irq provided");
		return irq;
	}

	hcd = usb_create_hcd(&ohci_platform_hc_driver, &dev->dev,
			dev_name(&dev->dev));
	if (!hcd)
		return -ENOMEM;

	platform_set_drvdata(dev, hcd);
	dev->dev.platform_data = pdata;
	priv = hcd_to_ohci_priv(hcd);
	ohci = hcd_to_ohci(hcd);

	pm_runtime_set_active(&dev->dev);
	pm_runtime_enable(&dev->dev);
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

	platform_set_drvdata(dev, hcd);

	return err;

err_power:
	if (pdata->power_off)
		pdata->power_off(dev);
err_reset:
	// pm_runtime_disable(&dev->dev);
	// reset_control_assert(priv->resets);
err_put_clks:
	// while (--clk >= 0)
	// 	clk_put(priv->clks[clk]);
err_put_hcd:
	if (pdata == &ohci_platform_defaults)
		dev->dev.platform_data = NULL;

	usb_put_hcd(hcd);

	return err;
}

static int ohci_platform_remove(struct platform_device *dev)
{
	return 0;
}

static const struct of_device_id ohci_platform_ids[] = {
	{ .compatible = "generic-ohci", },
	{ }
};
MODULE_DEVICE_TABLE(of, ohci_platform_ids);

static const struct platform_device_id ohci_platform_table[] = {
	{ "ohci-platform", 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, ohci_platform_table);

static struct platform_driver ohci_platform_driver = {
	.id_table	= ohci_platform_table,
	.probe		= ohci_platform_probe,
	.remove		= ohci_platform_remove,
	.driver		= {
		.name	= "ohci-platform",
		.of_match_table = ohci_platform_ids,
	}
};

int ohci_platform_init(void)
{
	if (usb_disabled())
		return -ENODEV;

	pr_info("%s: " DRIVER_DESC "\n", hcd_name);

	ohci_init_driver(&ohci_platform_hc_driver, &platform_overrides);
	return platform_driver_register(&ohci_platform_driver);
}

void ohci_platform_cleanup(void)
{
	platform_driver_unregister(&ohci_platform_driver);
}
