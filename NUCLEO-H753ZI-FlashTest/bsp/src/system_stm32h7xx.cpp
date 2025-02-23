#include "stm32h7xx.h"

extern "C" void system_init();

uint32_t SystemCoreClock = 64000000;

extern "C" uint32_t SystemD2Clock;
extern "C" const uint8_t D1CorePrescTable[16];

uint32_t SystemD2Clock   = 64000000;
const uint8_t D1CorePrescTable[16] = { 0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9 };

uint32_t usart_1_clk;
uint32_t usart_2_clk;
uint32_t usart_3_clk;
uint32_t usart_4_clk;
uint32_t usart_5_clk;
uint32_t usart_6_clk;
uint32_t usart_7_clk;
uint32_t usart_8_clk;

uint32_t APB1_Timer_clk;
uint32_t APB2_Timer_clk;

uint32_t ADC_clk;
namespace
{
  void enable_VOS1()
  {
    PWR->CPUCR |= PWR_CPUCR_RUN_D3;

    PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS_Msk) | ((0b11 << PWR_D3CR_VOS_Pos) & PWR_D3CR_VOS_Msk);
    while ((PWR->D3CR & PWR_D3CR_VOSRDY) == 0)
    {
    }
  }

  void enable_VOS0()
  {
    enable_VOS1();
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;

    while ((PWR->D3CR & PWR_D3CR_VOSRDY) == 0)
    {
    }
  }

  void enable_LDO()
  {
    PWR->CR3 = PWR_CR3_LDOEN;
    while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0)
    {
    }
  }

  void enable_FPU()
  {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10 * 2)) | (3UL << (11 * 2))); /* set CP10 and CP11 Full Access */
#endif
  }
}    // namespace

void system_init(void)
{
  enable_LDO();
  enable_FPU();
  enable_VOS0();

  // Activate HSI Clock 64 MHz and HSE Clock in byepassmode
  RCC->CR |= RCC_CR_HSION | RCC_CR_HSEBYP | RCC_CR_HSEON;

  // Wait until HSI is ready
  while ((RCC->CR & RCC_CR_HSIRDY_Msk) == 0)
  {
  }

  // set sys_ck to HSI
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | (0b000 << RCC_CFGR_SW_Pos);
  while (((RCC->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos) != 0b000)
  {
  }

  // Disable PLL 1,2,3
  RCC->CR &= ~(RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);

  // Changet HSI divider to 1
  RCC->CR &= ~RCC_CR_HSIDIV_Msk;

  // Now sys clock is on 64 MHz
  constexpr uint32_t hsi_clk = 64000000;
  // Setup PLLs
  constexpr uint32_t div_M1 = 32;
  constexpr uint32_t div_N1 = 480;
  constexpr uint32_t div_P1 = 2;
  constexpr uint32_t div_Q1 = 24;
  constexpr uint32_t div_R1 = 2;

  constexpr uint32_t div_M2 = 32;
  constexpr uint32_t div_N2 = 100;
  constexpr uint32_t div_P2 = 2;
  constexpr uint32_t div_Q2 = 2;
  constexpr uint32_t div_R2 = 2;

  constexpr uint32_t div_M3 = 32;    // Disabled
  constexpr uint32_t div_N3 = 129;
  constexpr uint32_t div_P3 = 2;
  constexpr uint32_t div_Q3 = 2;
  constexpr uint32_t div_R3 = 2;

  // clang-format off
  RCC->PLLCKSELR = (( div_M3 << RCC_PLLCKSELR_DIVM3_Pos)  & RCC_PLLCKSELR_DIVM3_Msk)
                 | (( div_M2 << RCC_PLLCKSELR_DIVM2_Pos)  & RCC_PLLCKSELR_DIVM2_Msk)
                 | (( div_M1 << RCC_PLLCKSELR_DIVM1_Pos)  & RCC_PLLCKSELR_DIVM1_Msk)
                 | (( 0 << RCC_PLLCKSELR_PLLSRC_Pos) & RCC_PLLCKSELR_PLLSRC_Msk);

  RCC->PLL1DIVR  = (( ( div_R1 - 1 ) << RCC_PLL1DIVR_R1_Pos)  & RCC_PLL1DIVR_R1_Msk)
                 | (( ( div_Q1 - 1 ) << RCC_PLL1DIVR_Q1_Pos)  & RCC_PLL1DIVR_Q1_Msk)
                 | (( ( div_P1 - 1 ) << RCC_PLL1DIVR_P1_Pos)  & RCC_PLL1DIVR_P1_Msk)
                 | (( ( div_N1 - 1 ) << RCC_PLL1DIVR_N1_Pos)  & RCC_PLL1DIVR_N1_Msk);

  RCC->PLL1FRACR  = ((  0 << RCC_PLL1FRACR_FRACN1_Pos)  & RCC_PLL1FRACR_FRACN1_Msk);


  RCC->PLL2DIVR  = (( ( div_R2 - 1 ) << RCC_PLL2DIVR_R2_Pos)  & RCC_PLL2DIVR_R2_Msk)
                 | (( ( div_Q2 - 1 ) << RCC_PLL2DIVR_Q2_Pos)  & RCC_PLL2DIVR_Q2_Msk)
                 | (( ( div_P2 - 1 ) << RCC_PLL2DIVR_P2_Pos)  & RCC_PLL2DIVR_P2_Msk)
                 | (( ( div_N2 - 1 ) << RCC_PLL2DIVR_N2_Pos)  & RCC_PLL2DIVR_N2_Msk);

  RCC->PLL2FRACR  = ((  0 << RCC_PLL2FRACR_FRACN2_Pos)  & RCC_PLL2FRACR_FRACN2_Msk);


  RCC->PLL3DIVR  = (( ( div_R3 - 1 ) << RCC_PLL3DIVR_R3_Pos)  & RCC_PLL3DIVR_R3_Msk)
                 | (( ( div_Q3 - 1 ) << RCC_PLL3DIVR_Q3_Pos)  & RCC_PLL3DIVR_Q3_Msk)
                 | (( ( div_P3 - 1 ) << RCC_PLL3DIVR_P3_Pos)  & RCC_PLL3DIVR_P3_Msk)
                 | (( ( div_N3 - 1 ) << RCC_PLL3DIVR_N3_Pos)  & RCC_PLL3DIVR_N3_Msk);

  RCC->PLL3FRACR  = ((  0 << RCC_PLL3FRACR_FRACN3_Pos)  & RCC_PLL3FRACR_FRACN3_Msk);

  
  RCC->PLLCFGR   = ((   0 << RCC_PLLCFGR_DIVR3EN_Pos)     & RCC_PLLCFGR_DIVR3EN_Msk)
                 | ((   0 << RCC_PLLCFGR_DIVQ3EN_Pos)     & RCC_PLLCFGR_DIVQ3EN_Msk)
                 | ((   0 << RCC_PLLCFGR_DIVP3EN_Pos)     & RCC_PLLCFGR_DIVP3EN_Msk)
                 | ((   0 << RCC_PLLCFGR_DIVR2EN_Pos)     & RCC_PLLCFGR_DIVR2EN_Msk)
                 | ((   1 << RCC_PLLCFGR_DIVQ2EN_Pos)     & RCC_PLLCFGR_DIVQ2EN_Msk)
                 | ((   1 << RCC_PLLCFGR_DIVP2EN_Pos)     & RCC_PLLCFGR_DIVP2EN_Msk)
                 | ((   0 << RCC_PLLCFGR_DIVR1EN_Pos)     & RCC_PLLCFGR_DIVR1EN_Msk)
                 | ((   1 << RCC_PLLCFGR_DIVQ1EN_Pos)     & RCC_PLLCFGR_DIVQ1EN_Msk)
                 | ((   1 << RCC_PLLCFGR_DIVP1EN_Pos)     & RCC_PLLCFGR_DIVP1EN_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL3RGE_Pos)     & RCC_PLLCFGR_PLL3RGE_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL3VCOSEL_Pos)  & RCC_PLLCFGR_PLL3VCOSEL_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL3FRACEN_Pos)  & RCC_PLLCFGR_PLL3FRACEN_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL2RGE_Pos)     & RCC_PLLCFGR_PLL2RGE_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL2VCOSEL_Pos)  & RCC_PLLCFGR_PLL2VCOSEL_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL2FRACEN_Pos)  & RCC_PLLCFGR_PLL2FRACEN_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL1RGE_Pos)     & RCC_PLLCFGR_PLL1RGE_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL1VCOSEL_Pos)  & RCC_PLLCFGR_PLL1VCOSEL_Msk)
                 | ((   0 << RCC_PLLCFGR_PLL1FRACEN_Pos)  & RCC_PLLCFGR_PLL1FRACEN_Msk);
  // clang-format on

  // Wait until HSE is ready
  while ((RCC->CR & RCC_CR_HSERDY) == 0)
  {
  }

  // Enable Pll 1 and 2
  RCC->CR |= RCC_CR_PLL1ON | RCC_CR_PLL2ON;

  while ((RCC->CR & RCC_CR_PLL1RDY) == 0)
  {
  }

  while ((RCC->CR & RCC_CR_PLL2RDY) == 0)
  {
  }

  // config prescaler and clk switches
  // clang-format off
  RCC->D1CFGR = ((   0b0000 << RCC_D1CFGR_D1CPRE_Pos)     & RCC_D1CFGR_D1CPRE_Msk)
              | ((    0b100 << RCC_D1CFGR_D1PPRE_Pos)     & RCC_D1CFGR_D1PPRE_Msk)
              | ((   0b1000 << RCC_D1CFGR_HPRE_Pos)       & RCC_D1CFGR_HPRE_Msk);

  RCC->D2CFGR = ((    0b100 << RCC_D2CFGR_D2PPRE2_Pos)    & RCC_D2CFGR_D2PPRE2_Msk)
              | ((    0b100 << RCC_D2CFGR_D2PPRE1_Pos)    & RCC_D2CFGR_D2PPRE1_Msk);

  RCC->D3CFGR = ((    0b100 << RCC_D3CFGR_D3PPRE_Pos)    & RCC_D3CFGR_D3PPRE_Msk);

  RCC->D3CCIPR = 0;  // ADC -> clk PLL-2-p
  RCC->D2CCIP1R = ((    0b01 << RCC_D2CCIP1R_FDCANSEL_Pos)    & RCC_D2CCIP1R_FDCANSEL_Msk); // PLL1_Q -> FDCAN
  RCC->D2CCIP2R = 0;

  RCC->CFGR |=  ((    0b1 << RCC_CFGR_HRTIMSEL_Pos)    & RCC_CFGR_HRTIMSEL_Msk); // HRTIM -> CPU-Clk 480MHz

  // clang-format on

  // config flash waitstates and Programming delay
  // clang-format off
  FLASH->ACR = ((     0b10 << FLASH_ACR_WRHIGHFREQ_Pos) & FLASH_ACR_WRHIGHFREQ_Msk)
               | ((      4 << FLASH_ACR_LATENCY_Pos)    & FLASH_ACR_LATENCY_Msk);
  // clang-format on

  // set sys_ck to PLL
  RCC->CFGR = ((RCC->CFGR & ~RCC_CFGR_SW_Msk) | 0b011 << RCC_CFGR_SW_Pos);
  while (((RCC->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos) != 0b011)
  {
  }

  constexpr uint32_t d1_c_pre = 1;
  constexpr uint32_t h_pre    = 2;
  // constexpr uint32_t d1_p_pre     = 2;
  constexpr uint32_t d2_p_pre_1 = 2;
  constexpr uint32_t d2_p_pre_2 = 2;
  // constexpr uint32_t d3_p_pre     = 2;

  constexpr uint32_t pll_in_clk = hsi_clk;

  constexpr uint32_t pll_1_vco_clk = (uint32_t)(((uint64_t)pll_in_clk * div_N1) / div_M1);
  constexpr uint32_t pll_2_vco_clk = (uint32_t)(((uint64_t)pll_in_clk * div_N2) / div_M2);
  // constexpr uint32_t pll_3_vco_clk = (uint32_t)(((uint64_t)pll_in_clk * div_N3) / div_M3);

  constexpr uint32_t pll_1_p_clk = pll_1_vco_clk / div_P1;
  // constexpr uint32_t pll_1_q_clk = pll_1_vco_clk / div_Q1;
  // constexpr uint32_t pll_1_r_clk = pll_1_vco_clk / div_R1;

  constexpr uint32_t pll_2_p_clk = pll_2_vco_clk / div_P2;
  // constexpr uint32_t pll_2_q_clk = pll_2_vco_clk / div_Q2;
  // constexpr uint32_t pll_2_r_clk = pll_2_vco_clk / div_R2;

  // constexpr uint32_t pll_3_p_clk = pll_3_vco_clk / div_P3;
  // constexpr uint32_t pll_3_q_clk = pll_3_vco_clk / div_Q3;
  // constexpr uint32_t pll_3_r_clk = pll_3_vco_clk / div_R3;

  constexpr uint32_t sys_clk      = pll_1_p_clk;
  constexpr uint32_t sys_d1_c_clk = sys_clk / d1_c_pre;
  constexpr uint32_t sys_h_clk    = sys_d1_c_clk / h_pre;

  // constexpr uint32_t h_clk_a = sys_h_clk;

  // constexpr uint32_t h_clk_1     = sys_h_clk;
  constexpr uint32_t p_clk_1     = sys_h_clk / d2_p_pre_1;
  constexpr uint32_t timer_clk_1 = (d2_p_pre_1 == 0) ? p_clk_1 : p_clk_1 * 2;

  // constexpr uint32_t h_clk_2     = sys_h_clk;
  constexpr uint32_t p_clk_2     = sys_h_clk / d2_p_pre_2;
  constexpr uint32_t timer_clk_2 = (d2_p_pre_2 == 0) ? p_clk_2 : p_clk_2 * 2;

  // constexpr uint32_t h_clk_3 = sys_h_clk;
  // constexpr uint32_t p_clk_3 = sys_h_clk / d1_p_pre;

  // constexpr uint32_t h_clk_4 = sys_h_clk;
  // constexpr uint32_t p_clk_4 = sys_h_clk / d3_p_pre;

  SystemCoreClock = sys_d1_c_clk;
  usart_1_clk     = p_clk_1;
  usart_2_clk     = p_clk_2;
  usart_3_clk     = p_clk_2;
  usart_4_clk     = p_clk_2;
  usart_5_clk     = p_clk_2;
  usart_6_clk     = p_clk_1;
  usart_7_clk     = p_clk_2;
  usart_8_clk     = p_clk_2;
  APB1_Timer_clk  = timer_clk_1;
  APB2_Timer_clk  = timer_clk_2;
  ADC_clk         = pll_2_p_clk;

  SCB_EnableICache();
  SCB_EnableDCache();
}

