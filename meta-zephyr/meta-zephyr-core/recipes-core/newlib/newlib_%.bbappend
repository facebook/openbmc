# BUILD_ASSERT(IS_ENABLED(_RETARGETABLE_LOCKING), "Retargetable locking must be enabled");

EXTRA_OECONF:append = " \
        --enable-newlib-retargetable-locking \
        "
