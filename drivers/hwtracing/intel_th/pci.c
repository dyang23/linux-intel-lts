// SPDX-License-Identifier: GPL-2.0
/*
 * Intel(R) Trace Hub pci driver
 *
 * Copyright (C) 2014-2015 Intel Corporation.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/pci.h>

#include "intel_th.h"

#define DRIVER_NAME "intel_th_pci"

#define BAR_MASK (BIT(TH_MMIO_CONFIG) | BIT(TH_MMIO_SW))

#define PCI_REG_NPKDSC	0x80
#define NPKDSC_TSACT	BIT(5)

static int intel_th_pci_activate(struct intel_th *th)
{
	struct pci_dev *pdev = to_pci_dev(th->dev);
	u32 npkdsc;
	int err;

	if (!INTEL_TH_CAP(th, tscu_enable))
		return 0;

	err = pci_read_config_dword(pdev, PCI_REG_NPKDSC, &npkdsc);
	if (!err) {
		npkdsc |= NPKDSC_TSACT;
		err = pci_write_config_dword(pdev, PCI_REG_NPKDSC, npkdsc);
	}

	if (err)
		dev_err(&pdev->dev, "failed to read NPKDSC register\n");

	return err;
}

static void intel_th_pci_deactivate(struct intel_th *th)
{
	struct pci_dev *pdev = to_pci_dev(th->dev);
	u32 npkdsc;
	int err;

	if (!INTEL_TH_CAP(th, tscu_enable))
		return;

	err = pci_read_config_dword(pdev, PCI_REG_NPKDSC, &npkdsc);
	if (!err) {
		npkdsc |= NPKDSC_TSACT;
		err = pci_write_config_dword(pdev, PCI_REG_NPKDSC, npkdsc);
	}

	if (err)
		dev_err(&pdev->dev, "failed to read NPKDSC register\n");
}
/*
 * PCI Configuration Registers
 */
enum {
	REG_PCI_NPKDSC		= 0x80, /* NPK Device Specific Control */
	REG_PCI_NPKDSD		= 0x90, /* NPK Device Specific Defeature */
};

/* Trace Hub software reset */
#define NPKDSC_RESET	BIT(1)

/* Force On */
#define NPKDSD_FON	BIT(0)

static void intel_th_pci_reset(struct intel_th *th)
{
	struct pci_dev *pdev = container_of(th->dev, struct pci_dev, dev);
	u32 val;

	/* Software reset */
	pci_read_config_dword(pdev, REG_PCI_NPKDSC, &val);
	val |= NPKDSC_RESET;
	pci_write_config_dword(pdev, REG_PCI_NPKDSC, val);

	/* Always set FON for S0ix flow */
	pci_read_config_dword(pdev, REG_PCI_NPKDSD, &val);
	val |= NPKDSD_FON;
	pci_write_config_dword(pdev, REG_PCI_NPKDSD, val);
}

static int intel_th_pci_probe(struct pci_dev *pdev,
			      const struct pci_device_id *id)
{
	struct intel_th_drvdata *drvdata = (void *)id->driver_data;
	struct intel_th *th;
	int err;

	err = pcim_enable_device(pdev);
	if (err)
		return err;

	err = pcim_iomap_regions_request_all(pdev, BAR_MASK, DRIVER_NAME);
	if (err)
		return err;

	th = intel_th_alloc(&pdev->dev, drvdata, pdev->resource,
			    DEVICE_COUNT_RESOURCE, pdev->irq, intel_th_pci_reset);
	if (IS_ERR(th))
		return PTR_ERR(th);

	th->activate   = intel_th_pci_activate;
	th->deactivate = intel_th_pci_deactivate;

	pci_set_master(pdev);

	return 0;
}

static void intel_th_pci_remove(struct pci_dev *pdev)
{
	struct intel_th *th = pci_get_drvdata(pdev);

	intel_th_free(th);
}

static const struct intel_th_drvdata intel_th_2x = {
	.tscu_enable	= 1,
};

static const struct pci_device_id intel_th_pci_id_table[] = {
	{
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x9d26),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa126),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Apollo Lake */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x5a8e),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Broxton */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x0a80),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Broxton B-step */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x1a8e),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Kaby Lake PCH-H */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa2a6),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Denverton */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x19e1),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Lewisburg PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa1a6),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Lewisburg PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa226),
		.driver_data = (kernel_ulong_t)0,
	},
	{
		/* Gemini Lake */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x318e),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Cannon Lake H */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa326),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Cannon Lake LP */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x9da6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Cedar Fork PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x18e1),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Ice Lake PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x34a6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Comet Lake */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x02a6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Comet Lake PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x06a6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Comet Lake PCH-V */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa3a6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Ice Lake NNPI */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x45c5),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Ice Lake CPU */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x8a29),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Tiger Lake CPU */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x9a33),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Tiger Lake PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa0a6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Jasper Lake PCH */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x4da6),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{
		/* Elkhart Lake */
		PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x4b26),
		.driver_data = (kernel_ulong_t)&intel_th_2x,
	},
	{ 0 },
};

MODULE_DEVICE_TABLE(pci, intel_th_pci_id_table);

static int intel_th_suspend(struct device *dev)
{
	/*
	 * Stub the call to avoid disabling the device.
	 * Suspend is fully handled by firmwares.
	 */
	return 0;
}

static int intel_th_resume(struct device *dev)
{
	/* Firmwares have already restored the device state. */
	return 0;
}

static const struct dev_pm_ops intel_th_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(intel_th_suspend,
				intel_th_resume)
};

static struct pci_driver intel_th_pci_driver = {
	.name		= DRIVER_NAME,
	.id_table	= intel_th_pci_id_table,
	.probe		= intel_th_pci_probe,
	.remove		= intel_th_pci_remove,
	.driver         = {
		.pm     = &intel_th_pm_ops,
	},
};

module_pci_driver(intel_th_pci_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Intel(R) Trace Hub PCI controller driver");
MODULE_AUTHOR("Alexander Shishkin <alexander.shishkin@intel.com>");
