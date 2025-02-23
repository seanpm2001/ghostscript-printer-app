//
// Ghostscript (Foomatic) Printer Application based on PAPPL and
// libpappl-retrofit
//
// Copyright © 2020-2021 by Till Kamppeter.
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers...
//

#include <pappl-retrofit.h>


//
// Constants...
//

// Name and version

#define SYSTEM_NAME "Ghostscript Printer Application"
#define SYSTEM_PACKAGE_NAME "ghostscript-printer-app"
#define SYSTEM_VERSION_STR "1.0"
#define SYSTEM_VERSION_ARR_0 1
#define SYSTEM_VERSION_ARR_1 0
#define SYSTEM_VERSION_ARR_2 0
#define SYSTEM_VERSION_ARR_3 0
#define SYSTEM_WEB_IF_FOOTER "Copyright &copy; 2021 by Till Kamppeter. Provided under the terms of the <a href=\"https://www.apache.org/licenses/LICENSE-2.0\">Apache License 2.0</a>."

// Test page

#define TESTPAGE "testpage.pdf"


//
// Spooling conversions
//

// When using the fxlinuxprint driver for the non-IPP-driverless PDF
// printers from Fuji Xerox convert PDF with the driver's "pdftopdffx"
// filter

// Raster input does not need extra conversion rules, here we make use
// of the PostScript input support of the driver

static pr_spooling_conversion_t ghostscript_convert_pdf_to_fx_pdf =
{
  "application/pdf",
  "application/vnd.cups-pdfprintfx",
  2,
  {
    {
      cfFilterPDFToPDF,
      NULL,
      "pdftopdf"
    },
    {
      ppdFilterExternalCUPS,
      &((cf_filter_external_t){
	"pdftopdffx",
	0,
	0,
	NULL,
	NULL
      }),
      "pdftopdffx"
    }
  }
};


//
// 'ghostscript_autoadd()' - Auto-add printers.
//

const char *			        // O - Driver name or `NULL` for none
ghostscript_autoadd(const char *device_info,	// I - Device name (unused)
	     const char *device_uri,	// I - Device URI (unused)
	     const char *device_id,	// I - IEEE-1284 device ID
	     void       *data)          // I - Global data
{
  pr_printer_app_global_data_t *global_data =
    (pr_printer_app_global_data_t *)data;
  const char	*ret = NULL;		// Return value


  (void)device_info;
  (void)device_uri;

  if (device_id == NULL || global_data == NULL)
    return (NULL);

  // 
  // Find the best-matching PPD file to expicitly support our printer model
  if (!((ret = prBestMatchingPPD(device_id, global_data)) != 0 ||
	// No dedicated support for this model, look at the COMMAND
	// SET (CMD) key in the device ID for the list of printer
	// languages and select a generic driver if we find a
	// supported language
	(prSupportsPostScript(device_id) &&
	 (ret = prBestMatchingPPD("MFG:Generic;MDL:PostScript Printer;",
				     global_data)) != 0) ||
	(prSupportsPCLXL(device_id) &&
	 (ret = prBestMatchingPPD("MFG:Generic;MDL:PCL 6/PCL XL Printer;",
				   global_data)) != 0) ||
	(prSupportsPCL5c(device_id) &&
	 (ret = prBestMatchingPPD("MFG:Generic;MDL:PCL 5c Printer;",
				     global_data)) != 0) ||
	(prSupportsPCL5(device_id) &&
	 (ret = prBestMatchingPPD("MFG:Generic;MDL:PCL 5e Printer;",
				     global_data)) != 0)))
    // Printer does not support our PDL, it is not supported by this
    // Printer Application
    ret = NULL;

  return (ret);
}


//
// 'main()' - Main entry for the ghostscript-printer-app.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  cups_array_t *spooling_conversions,
               *stream_formats,
               *driver_selection_regex_list;

  // Array of spooling conversions, most desirables first
  //
  // Here we prefer not converting into another format
  // Keeping vector formats (like PS -> PDF) is usually more desirable
  // but as many printers have buggy PS interpreters we prefer converting
  // PDF to Raster and not to PS
  spooling_conversions = cupsArrayNew(NULL, NULL);

  // Special conversion rule for fxlinuxprint driver (Fuji Xerox
  // non-IPP-driverless PDF printers)
  cupsArrayAdd(spooling_conversions, &ghostscript_convert_pdf_to_fx_pdf);

  // Standard conversion rules
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PDF_TO_PDF);
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PDF_TO_RASTER);
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PDF_TO_PS);
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PS_TO_PS);
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PS_TO_PDF);
  cupsArrayAdd(spooling_conversions, (void *)&PR_CONVERT_PS_TO_RASTER);

  // Array of stream formats, most desirables first
  //
  // PDF comes last because it is generally not streamable.
  // PostScript comes second as it is Ghostscript's streamable
  // input format.
  stream_formats = cupsArrayNew(NULL, NULL);
  cupsArrayAdd(stream_formats, (void *)&PR_STREAM_CUPS_RASTER);
  cupsArrayAdd(stream_formats, (void *)&PR_STREAM_POSTSCRIPT);
  cupsArrayAdd(stream_formats, (void *)&PR_STREAM_PDF);

  // Array of regular expressions for driver priorization
  // For some printers the "(recommended)" is missing ...
  driver_selection_regex_list = cupsArrayNew(NULL, NULL);
  cupsArrayAdd(driver_selection_regex_list, "-recommended-");
  cupsArrayAdd(driver_selection_regex_list, "-pdf-");
  cupsArrayAdd(driver_selection_regex_list, "-postscript-");
  cupsArrayAdd(driver_selection_regex_list, "-pxlcolor-");
  cupsArrayAdd(driver_selection_regex_list, "-hpijs-pcl-5-c-");
  cupsArrayAdd(driver_selection_regex_list, "-cljet-5-");
  cupsArrayAdd(driver_selection_regex_list, "-pxl-");
  cupsArrayAdd(driver_selection_regex_list, "-pxlmono-");
  cupsArrayAdd(driver_selection_regex_list, "-lj-5-gray-");
  cupsArrayAdd(driver_selection_regex_list, "-lj-5-mono-");
  cupsArrayAdd(driver_selection_regex_list, "-hl-1250-");
  cupsArrayAdd(driver_selection_regex_list, "-hl-7-x-0-");
  cupsArrayAdd(driver_selection_regex_list, "-hpijs-pcl-5-e-");
  cupsArrayAdd(driver_selection_regex_list, "-pcl-5-");
  cupsArrayAdd(driver_selection_regex_list, "-ljet-4-d-");
  cupsArrayAdd(driver_selection_regex_list, "-ljet-4-");
  cupsArrayAdd(driver_selection_regex_list, "-lj-4-dith");
  cupsArrayAdd(driver_selection_regex_list, "-ljet-3-d-");
  cupsArrayAdd(driver_selection_regex_list, "-ljet-3-");
  cupsArrayAdd(driver_selection_regex_list, "-laserjet-");
  cupsArrayAdd(driver_selection_regex_list, "-ljet-2-p-");
  cupsArrayAdd(driver_selection_regex_list, "-ljetplus-");
  cupsArrayAdd(driver_selection_regex_list, "-pcl-3-");

  // Configuration record of the Printer Application
  pr_printer_app_config_t printer_app_config =
  {
    SYSTEM_NAME,              // Display name for Printer Application
    SYSTEM_PACKAGE_NAME,      // Package/executable name
    SYSTEM_VERSION_STR,       // Version as a string
    {
      SYSTEM_VERSION_ARR_0,   // Version 1st number
      SYSTEM_VERSION_ARR_1,   //         2nd
      SYSTEM_VERSION_ARR_2,   //         3rd
      SYSTEM_VERSION_ARR_3    //         4th
    },
    SYSTEM_WEB_IF_FOOTER,     // Footer for web interface (in HTML)
    PR_COPTIONS_QUERY_PS_DEFAULTS | // pappl-retrofit special features to be
    PR_COPTIONS_NO_GENERIC_DRIVER | // used
#if !SNAP
    PR_COPTIONS_USE_ONLY_MATCHING_NICKNAMES |
#endif // !SNAP
    PR_COPTIONS_NO_PAPPL_BACKENDS |
    PR_COPTIONS_CUPS_BACKENDS,
    ghostscript_autoadd,      // Auto-add (driver assignment) callback
    prIdentify,              // Printer identify callback
    prTestPage,              // Test page print callback
    NULL,                     // No extra setup steps for the system
    prSetupDeviceSettingsPage, // Set up "Device Settings" printer web
                              // interface page
    spooling_conversions,     // Array of data format conversion rules for
                              // printing in spooling mode
    stream_formats,           // Arrray for stream formats to be generated
                              // when printing in streaming mode
    "",                       // CUPS backends to be ignored
    "snmp,dnssd,usb",         // CUPS backends to be used exclusively
                              // If empty all but the ignored backends are used
    TESTPAGE,                 // Test page (printable file), used by the
                              // standard test print callback prTestPage()
    " +Foomatic/(.+)$| +(PDF|PXL|PCL5)$",
                              // Regular expression to separate the
                              // extra information after make/model in
                              // the PPD's *NickName. Also extracts a
                              // contained driver name (by using
                              // parentheses)
    driver_selection_regex_list
                              // Regular expression for the driver
                              // auto-selection to prioritize a driver
                              // when there is more than one for a
                              // given printer. If a regular
                              // expression matches on the driver
                              // name, the driver gets priority. If
                              // there is more than one matching
                              // driver, the driver name on which the
                              // earlier regular expression in the
                              // list matches, gets the priority.
  };

  return (prRetroFitPrinterApp(&printer_app_config, argc, argv));
}
