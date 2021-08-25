PACKAGECONFIG[tui] = "--enable-tui,--disable-tui"
PACKAGECONFIG_append = " tui"
EXTRA_OECONF_remove = "--disable-tui"
