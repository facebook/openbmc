PACKAGECONFIG[tui] = "--enable-tui,--disable-tui"
PACKAGECONFIG:append = " tui"
EXTRA_OECONF:remove = "--disable-tui"
