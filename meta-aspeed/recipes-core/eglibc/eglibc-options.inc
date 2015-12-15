def eglibc_cfg(feature, tokens, cnf):
    if type(tokens) == type(""):
        tokens = [tokens]
    if feature:
        cnf.extend([token + '=y' for token in tokens])
    else:
        for token in tokens:
            cnf.extend([token + '=n'])
            if token == 'OPTION_EGLIBC_NSSWITCH':
                cnf.extend(["OPTION_EGLIBC_NSSWITCH_FIXED_CONFIG=\"${S}/nss/nsswitch.conf\""])
                cnf.extend(["OPTION_EGLIBC_NSSWITCH_FIXED_FUNCTIONS=\"${S}/nss/fixed-nsswitch.functions\""])

# Map distro features to eglibc options settings
def features_to_eglibc_settings(d):
    cnf = ([])

    ipv4 = base_contains('DISTRO_FEATURES', 'ipv4', True, False, d)
    ipv6 = base_contains('DISTRO_FEATURES', 'ipv6', True, False, d)
    libc_backtrace = base_contains('DISTRO_FEATURES', 'libc-backtrace', True, False, d) 
    libc_big_macros = base_contains('DISTRO_FEATURES', 'libc-big-macros', True, False, d)
    libc_bsd = base_contains('DISTRO_FEATURES', 'libc-bsd', True, False, d)
    libc_cxx_tests = base_contains('DISTRO_FEATURES', 'libc-cxx-tests', True, False, d)
    libc_catgets = base_contains('DISTRO_FEATURES', 'libc-catgets', True, False, d)
    libc_charsets = base_contains('DISTRO_FEATURES', 'libc-charsets', True, False, d)
    libc_crypt = base_contains('DISTRO_FEATURES', 'libc-crypt', True, False, d)
    libc_crypt_ufc = base_contains('DISTRO_FEATURES', 'libc-crypt-ufc', True, False, d)
    libc_db_aliases = base_contains('DISTRO_FEATURES', 'libc-db-aliases', True, False, d)
    libc_envz = base_contains('DISTRO_FEATURES', 'libc-envz', True, False, d)
    libc_fcvt = base_contains('DISTRO_FEATURES', 'libc-fcvt', True, False, d)
    libc_fmtmsg = base_contains('DISTRO_FEATURES', 'libc-fmtmsg', True, False, d)
    libc_fstab = base_contains('DISTRO_FEATURES', 'libc-fstab', True, False, d)
    libc_ftraverse = base_contains('DISTRO_FEATURES', 'libc-ftraverse', True, False, d)
    libc_getlogin = base_contains('DISTRO_FEATURES', 'libc-getlogin', True, False, d)
    libc_idn = base_contains('DISTRO_FEATURES', 'libc-idn', True, False, d)
    libc_inet_anl = base_contains('DISTRO_FEATURES', 'libc-inet-anl', True, False, d)
    libc_libm = base_contains('DISTRO_FEATURES', 'libc-libm', True, False, d)
    libc_locales = base_contains('DISTRO_FEATURES', 'libc-locales', True, False, d)
    libc_locale_code = base_contains('DISTRO_FEATURES', 'libc-locale-code', True, False, d)
    libc_memusage = base_contains('DISTRO_FEATURES', 'libc-memusage', True, False, d)
    libc_nis = base_contains('DISTRO_FEATURES', 'libc-nis', True, False, d)
    libc_nsswitch = base_contains('DISTRO_FEATURES', 'libc-nsswitch', True, False, d)
    libc_rcmd = base_contains('DISTRO_FEATURES', 'libc-rcmd', True, False, d)
    libc_rtld_debug = base_contains('DISTRO_FEATURES', 'libc-rtld-debug', True, False, d)
    libc_spawn = base_contains('DISTRO_FEATURES', 'libc-spawn', True, False, d)
    libc_streams = base_contains('DISTRO_FEATURES', 'libc-streams', True, False, d)
    libc_sunrpc = base_contains('DISTRO_FEATURES', 'libc-sunrpc', True, False, d)
    libc_utmp = base_contains('DISTRO_FEATURES', 'libc-utmp', True, False, d)
    libc_utmpx = base_contains('DISTRO_FEATURES', 'libc-utmpx', True, False, d)
    libc_wordexp = base_contains('DISTRO_FEATURES', 'libc-wordexp', True, False, d)
    libc_posix_clang_wchar = base_contains('DISTRO_FEATURES', 'libc-posix-clang-wchar', True, False, d)
    libc_posix_regexp = base_contains('DISTRO_FEATURES', 'libc-posix-regexp', True, False, d)
    libc_posix_regexp_glibc = base_contains('DISTRO_FEATURES', 'libc-posix-regexp-glibc', True, False, d)
    libc_posix_wchar_io = base_contains('DISTRO_FEATURES', 'libc-posix-wchar-io', True, False, d)

    # arrange the dependencies among eglibc configuable options according to file option-groups.def from eglibc source code
    new_dep = True
    while new_dep:
        new_dep = False

        if ipv6 and not ipv4:
            new_dep = True
            ipv4 = True

        if ipv4 and not libc_nsswitch:
            new_dep = True
            libc_nsswitch = True

        if libc_cxx_tests:
            if not libc_posix_wchar_io:
                new_dep = True
                libc_posix_wchar_io = True
            if not libc_libm:
                new_dep = True
                libc_libm = True

        if libc_catgets and not libc_locale_code:
            new_dep = True
            libc_locale_code = True

        if libc_crypt_ufc and not libc_crypt:
            new_dep = True
            libc_crypt = True

        if libc_getlogin and not libc_utmp:
            new_dep = True
            libc_utmp = True

        if libc_inet_anl and not ipv4:
            new_dep = True
            ipv4 = True

        if libc_locale_code and not libc_posix_clang_wchar:
            new_dep = True
            libc_posix_clang_wchar = True

        if libc_nis:
            if not ipv4:
                new_dep = True
                ipv4 = True
            if not libc_sunrpc:
                new_dep = True
                libc_sunrpc = True

        if libc_rcmd and not ipv4:
            new_dep = True
            ipv4 = True

        if libc_sunrpc and not ipv4:
            new_dep = True
            ipv4 = True

        if libc_utmpx and not libc_utmp:
            new_dep = True
            libc_utmp = True

        if libc_posix_regexp_glibc and not libc_posix_regexp:
            new_dep = True
            libc_posix_regexp = True

        if libc_posix_wchar_io and not libc_posix_clang_wchar:
            new_dep = True
            libc_posix_clang_wchar = True

    eglibc_cfg(ipv6, 'OPTION_EGLIBC_ADVANCED_INET6', cnf)
    eglibc_cfg(libc_backtrace, 'OPTION_EGLIBC_BACKTRACE', cnf)
    eglibc_cfg(libc_big_macros, 'OPTION_EGLIBC_BIG_MACROS', cnf)
    eglibc_cfg(libc_bsd, 'OPTION_EGLIBC_BSD', cnf)
    eglibc_cfg(libc_cxx_tests, 'OPTION_EGLIBC_CXX_TESTS', cnf)
    eglibc_cfg(libc_catgets, 'OPTION_EGLIBC_CATGETS', cnf)
    eglibc_cfg(libc_charsets, 'OPTION_EGLIBC_CHARSETS', cnf)
    eglibc_cfg(libc_crypt, 'OPTION_EGLIBC_CRYPT', cnf)
    eglibc_cfg(libc_crypt_ufc, 'OPTION_EGLIBC_CRYPT_UFC', cnf)
    eglibc_cfg(libc_db_aliases, 'OPTION_EGLIBC_DB_ALIASES', cnf)
    eglibc_cfg(libc_envz, 'OPTION_EGLIBC_ENVZ', cnf)
    eglibc_cfg(libc_fcvt, 'OPTION_EGLIBC_FCVT', cnf)
    eglibc_cfg(libc_fmtmsg, 'OPTION_EGLIBC_FMTMSG', cnf)
    eglibc_cfg(libc_fstab, 'OPTION_EGLIBC_FSTAB', cnf)
    eglibc_cfg(libc_ftraverse, 'OPTION_EGLIBC_FTRAVERSE', cnf)
    eglibc_cfg(libc_getlogin, 'OPTION_EGLIBC_GETLOGIN', cnf)
    eglibc_cfg(libc_idn, 'OPTION_EGLIBC_IDN', cnf)
    eglibc_cfg(ipv4, 'OPTION_EGLIBC_INET', cnf)
    eglibc_cfg(libc_inet_anl, 'OPTION_EGLIBC_INET_ANL', cnf)
    eglibc_cfg(libc_libm, 'OPTION_EGLIBC_LIBM', cnf)
    eglibc_cfg(libc_locales, 'OPTION_EGLIBC_LOCALES', cnf)
    eglibc_cfg(libc_locale_code, 'OPTION_EGLIBC_LOCALE_CODE', cnf)
    eglibc_cfg(libc_memusage, 'OPTION_EGLIBC_MEMUSAGE', cnf)
    eglibc_cfg(libc_nis, 'OPTION_EGLIBC_NIS', cnf)
    eglibc_cfg(libc_nsswitch, 'OPTION_EGLIBC_NSSWITCH', cnf)
    eglibc_cfg(libc_rcmd, 'OPTION_EGLIBC_RCMD', cnf)
    eglibc_cfg(libc_rtld_debug, 'OPTION_EGLIBC_RTLD_DEBUG', cnf)
    eglibc_cfg(libc_spawn, 'OPTION_EGLIBC_SPAWN', cnf)
    eglibc_cfg(libc_streams, 'OPTION_EGLIBC_STREAMS', cnf)
    eglibc_cfg(libc_sunrpc, 'OPTION_EGLIBC_SUNRPC', cnf)
    eglibc_cfg(libc_utmp, 'OPTION_EGLIBC_UTMP', cnf)
    eglibc_cfg(libc_utmpx, 'OPTION_EGLIBC_UTMPX', cnf)
    eglibc_cfg(libc_wordexp, 'OPTION_EGLIBC_WORDEXP', cnf)
    eglibc_cfg(libc_posix_clang_wchar, 'OPTION_POSIX_C_LANG_WIDE_CHAR', cnf)
    eglibc_cfg(libc_posix_regexp, 'OPTION_POSIX_REGEXP', cnf)
    eglibc_cfg(libc_posix_regexp_glibc, 'OPTION_POSIX_REGEXP_GLIBC', cnf)
    eglibc_cfg(libc_posix_wchar_io, 'OPTION_POSIX_WIDE_CHAR_DEVICE_IO', cnf)

    return "\n".join(cnf)
