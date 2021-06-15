#include "dzzy.h"

void keyboard_post_init_user(void) {
#if defined(__riscv)
    // Free B4 pin.
    AFIO->MAPR |= AFIO_PCF0_SWJ_CFG_NOJNTRST;
#endif
    // Customise these values to desired behaviour
    // debug_enable = true;
    // debug_matrix = true;
    // debug_keyboard = true;
    // debug_mouse=true;
}

#if defined(__riscv)

#    pragma GCC push_options
#    pragma GCC optimize("O0")

uintptr_t handle_trap(uintptr_t mcause, uintptr_t sp, uintptr_t mdcause, uintptr_t msubm) {
    uint32_t             m_epc        = __RV_CSR_READ(mepc);
    uint32_t             m_tval       = __RV_CSR_READ(mtval);
    uint32_t             m_bad        = __RV_CSR_READ(mbadaddr);
    CSR_MSTATUS_Type     m_status     = {.d = __RV_CSR_READ(mstatus)};
    CSR_MCAUSE_Type      m_cause      = {.d = mcause};
    CSR_MSAVESTATUS_Type m_savestatus = {.w = __RV_CSR_READ(CSR_MSAVESTATUS)};
    CSR_MSUBM_Type       m_subm       = {.d = msubm};

    (void)m_epc;
    (void)m_tval;
    (void)m_bad;
    (void)m_status;
    (void)m_cause;
    (void)m_savestatus;
    (void)m_subm;

    while (1)
        ;

    return 0;
}
#    pragma GCC pop_options

#endif