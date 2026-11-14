# Plano de engenharia reversa do PCSX2-360

Data: 2026-11-14

## Objetivo

Acelerar o port para Xbox 360 RGH/JTAG usando engenharia reversa comportamental, comparação com a implementação pública do PCSX2 e testes diferenciais. O executável final permanece `build/default.xex`; qualquer ELF necessário ao conversor é temporário e não é artefato de entrega.

## Limites e método

- Usar somente código-fonte público, documentação técnica, testes e dumps obtidos legalmente.
- Não contornar DRM, proteção anticópia, autenticação, nem extrair BIOS/firmware proprietário.
- Não copiar grandes blocos de código GPL para o projeto MPL-2.0. O PCSX2 é GPL-3.0+; antes de qualquer incorporação, realizar análise de compatibilidade de licença e preservar avisos. Preferir especificações comportamentais, interfaces independentes e reimplementação limpa.
- Para cada comportamento: formular hipótese, criar teste mínimo, comparar com referência, registrar diferença, corrigir e repetir.
- Não marcar como concluído sem teste executado e log guardado.

## Fontes públicas de referência

- PCSX2 upstream: https://github.com/PCSX2/pcsx2
- R5900 reset/exception e ciclo EE: `pcsx2/R5900.cpp`
- CP0/TLB e registradores: `pcsx2/COP0.cpp`, `pcsx2/COP0.h`
- DMA e endereçamento de tags: `pcsx2/Dmac.cpp`, `pcsx2/Dmac.h`
- Licença upstream: GPL-3.0+.

Os arquivos upstream são referências para comportamento, não uma autorização para copiar código.

## Descobertas iniciais da leitura de referência

### R5900 / CP0

A rotina de reset upstream configura o PC inicial do EE em `0xBFC00000`, configura Config/Status/PRid e zera estado de CPU, FPU e TLB. A rotina de exceção diferencia nível normal e nível 2, trata EXL, EPC e BD (branch delay), e escolhe vetores diferentes para interrupção, refill TLB e exceções gerais. O PCSX2 também mantém 48 entradas TLB no estado global do EE.

### DMA

O código de DMA upstream usa canais com registradores CHCR, MADR, QWC e TADR, tags DMA, habilitação global e status de erro. Os endereços DMA são tratados como endereços físicos; o código resolve RAM principal, scratchpad e regiões relacionadas a VU. O protótipo genérico de cópia do PS2Palace360 não substitui essa semântica.

## Plano de testes diferenciais

1. **CP0 reset:** valores de reset, PC, Status, Config, PRid, estado TLB.
2. **Exceções:** EPC, Cause.ExcCode, Cause.BD, EXL, vetores BEV ligado/desligado.
3. **TLB:** refill, invalid, modified, endereço virtual, ASID/global, PageMask e pares EntryLo.
4. **Delay slots:** branch tomado/não tomado, exceção no delay slot e EPC/BD.
5. **DMA:** registradores, alinhamento de 16 bytes, QWC, tags, canais e interrupções.
6. **Interrupções e scheduler:** ordem de eventos EE/IOP, latência e reentrada.
7. **Reprodutibilidade:** cada caso tem entrada, estado inicial, estado esperado, resultado observado e log.

## Próxima fatia de implementação

Não ampliar ainda para VU/GS. Primeiro estabilizar CP0/TLB e o caminho de exceções:
- auditar a representação atual de Status/Cause/EPC/BadVAddr;
- separar código de exceção e flags de Cause;
- implementar vetores e regra BEV/EXL com testes;
- rastrear BD/EPC corretamente no delay slot;
- adicionar casos TLB com EntryLo válido, dirty, global e ASID;
- validar no host antes de integrar com scheduler.

## Integração de Claude Opus

Não há integração Claude Opus conectada e a busca de plugins não encontrou um conector disponível. Claude Opus pode atuar como revisor externo se o usuário fornecer uma revisão/saída ou conectar um serviço compatível. Não afirmar que Claude foi executado nesta sessão. Usar prompts de auditoria com contexto e diffs pequenos, exigindo referências a arquivos/linhas e testes reproduzíveis; revisar manualmente cada sugestão antes de aplicá-la.

## Estado de validação

Este documento registra uma auditoria inicial de referência. Não significa que CP0/TLB foram implementados, que testes passaram, que a toolchain Xenon foi executada ou que `default.xex` foi produzido.
