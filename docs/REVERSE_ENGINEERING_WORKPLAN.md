# Plano de engenharia reversa — PCSX2-360

Data: 2026-11-14  
Alvo final: `build/default.xex`  
Estado: análise de referências iniciada; nenhuma nova compatibilidade de jogo é reivindicada.

## Objetivo

Acelerar a implementação do PS2 por análise comparativa de:
- documentação pública do hardware PS2 (Emotion Engine/R5900, CP0, TLB, DMA, VU, GIF, GS e IOP);
- comportamento observável de implementações de referência;
- testes diferenciais pequenos e reproduzíveis;
- código experimental já existente no PS2Palace360.

A engenharia reversa será limitada à interoperabilidade, precisão de hardware e compatibilidade de software. Não inclui extração ou redistribuição de BIOS, jogos, chaves, firmware ou conteúdo protegido.

## Referências de código consultadas

O PCSX2 upstream foi usado como referência comportamental e de arquitetura:
- `pcsx2/R5900.cpp`: reset do EE, estado inicial CP0, EPC/Cause, vetores de exceção, TLB miss e sincronização EE/IOP.
- `pcsx2/COP0.cpp` e `pcsx2/COP0.h`: semântica de Status/Config, contadores e operações CP0/TLB.
- `pcsx2/Dmac.cpp`: canais DMA, transferência, endereçamento físico, scratchpad, memória EE/VU e condições de erro.

Links:
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900.cpp
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/COP0.cpp
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/Dmac.cpp

**Licenciamento:** os arquivos consultados do PCSX2 declaram GPL-3.0+. Este plano registra observações arquiteturais, não copia implementação. Qualquer futura importação literal exige análise de licença, compatibilidade com a licença do destino e preservação de avisos de copyright. Preferir implementação independente baseada em especificações públicas e testes.

## Achados técnicos para o port

### CP0 e exceções
O PCSX2 mostra que o tratamento de exceção precisa considerar:
- Status.EXL e ERL;
- Status.BEV e seleção do vetor;
- EPC e Cause.BD quando a falha ocorre em delay slot;
- BadVAddr, Context e EntryHi em TLB miss;
- distinção entre refill, interrupt e exceção geral;
- vetores normais e bootstrap;
- interrupções e atualização de eventos/ciclos.

O protótipo PCSX2-360 ainda não cobre integralmente esses estados. A implementação de exceções de dados é parcial e requer testes, inclusive de delay slot e TLB refill.

### DMA
A referência PCSX2 evidencia que DMA não é apenas memcpy: há canais, CHCR/MADR/QWC/TADR, tags, endereçamento físico, scratchpad, memória VU, filas, habilitação global, erros de barramento e interação com dispositivos. O protótipo DMA do PS2Palace360 é apenas cópia genérica segura e não deve ser promovido como DMA PS2 real.

### Cronometria
O EE precisa coordenar ciclos, eventos e interrupções com o IOP e periféricos. Uma execução de instruções sem scheduler não basta para compatibilidade.

## Plano acelerado de trabalho

1. **CP0/R5900:** inventariar registradores, máscaras graváveis, reset values, MFC0/MTC0, TLBWI/TLBWR/TLBP/TLBR e exceções.
2. **Delay slots:** introduzir estado explícito de branch-delay e testar EPC/BD, branch-likely e exceções no delay slot.
3. **TLB:** separar refill, invalid, modified e address errors; validar EntryHi/Context/BadVAddr e permissões.
4. **Interrupções:** mapear linhas, máscaras, pending bits e integração com scheduler.
5. **DMA:** modelar canais e registradores reais; depois tag chains, prioridades e dispositivos.
6. **IOP/SIF:** sincronização e comunicação EE↔IOP.
7. **VU/GIF/GS:** microinstruções, transferência de dados e pipeline gráfico.
8. **Backend Xenon:** Xenos, áudio, input, filesystem e runtime; manter `default.xex` como único artefato final publicado.

## Critérios de aceitação por subsistema

- Referência de hardware/documento identificada.
- Modelo de estado e semântica descritos.
- Testes unitários para casos normais e limites.
- Testes diferenciais contra comportamento de referência quando possível.
- Host CI aprovado.
- Só então integrar com o scheduler.
- Builds Xenon e execução RGH/JTAG são marcos separados.

## Prompt de revisão independente para Claude Opus

Use este prompt em uma sessão Claude Opus com acesso ao repositório:

> Você é revisor de arquitetura de emuladores de hardware. Analise o repositório PCSX2-360 e compare os módulos EE atuais com a semântica pública do Emotion Engine/R5900 e com os pontos de referência do PCSX2 upstream: R5900.cpp, COP0.cpp e Dmac.cpp. Não copie trechos GPL. Faça uma auditoria clean-room baseada em comportamento e documentação. Liste bugs concretos com arquivo, função, cenário reproduzível e teste proposto. Priorize CP0, delay slots, TLB, exceções e interrupções. Não declare implementação completa sem testes. Proponha patches pequenos em C portátil, compatíveis com o build host e com a futura toolchain Xenon. Preserve o objetivo de gerar somente build/default.xex como executável final; qualquer ELF deve ser intermediário temporário. Não invente APIs ou resultados de build.

