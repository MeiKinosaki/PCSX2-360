# PCSX2-360

Projeto experimental de emulação de PlayStation 2 para Xbox 360 RGH/JTAG.

**Versão de desenvolvimento: v0.4.2b — em andamento (validação do novo snapshot pendente).**

**Progresso global estimado: 28%.** Marco v0.4b: serialização portátil little-endian v2 do estado EE, TLB de 48 entradas e RAM, com checksum e validação antes da restauração; o tamanho do registro TLB foi corrigido para 25 bytes e a validação de terminadores NUL da biblioteca foi corrigida. As duas alterações foram relidas no GitHub, mas ainda aguardam execução confirmada no CI. O avanço não representa uma máquina PS2 completa. Marco anterior v0.3b: integração experimental de CP0/TLB e PageMask variável; CP0.Context, BadVAddr, Context.BadVPN2 e EntryHi.VPN2 nas exceções TLB; distinção entre TLB miss e entrada inválida; rastreamento do branch proprietário do delay slot, EPC/Cause.BD em exceções e retomada no destino do desvio. A regressão host `make check` foi aprovada no GitHub Actions para o commit `a70db3a1bc87694124747736eba7bc63e1fbaa4a` (run `43644957989`). O resultado valida apenas os testes host incluídos, não a precisão integral do R5900 nem a compatibilidade com jogos.


## Objetivo do build

O executável final obrigatório é **`build/default.xex`**. O arquivo ELF, quando exigido pelo conversor `elf2xex`, é criado apenas como intermediário temporário durante o build e removido após a conversão. Não é artefato de entrega.

## Estado atual

- [x] Repositório e baseline de migração documentados.
- [x] Primeiros módulos experimentais EE importados do PS2Palace360.
- [x] Teste host EE importado.
- [x] Aviso de licença MIT original preservado em `LICENSES/PS2Palace360-MIT.txt`.
- [x] Makefile inicial com alvo `make xex` que entrega `build/default.xex`.
- [x] Workflow de regressão host e workflow manual para runner Xenon self-hosted.
- [x] Primeira rota de exceções para falhas de dados do EE: TLB miss, Modified, ADEL, ADES e DBE no caminho experimental.
- [x] Testes de regressão adicionados para TLB miss em leitura, acesso desalinhado e store em página somente leitura.
- [x] Interface CP0 experimental para BadVAddr, Status, Cause e EPC.
- [x] Decodificação parcial de MFC0/MTC0 e testes de transferência/máscaras de escrita.
- [x] Tabela TLB experimental ampliada para 48 slots e teste do índice 47.
- [x] Modelo arquitetural experimental separado com 48 pares de páginas de 4 KiB (EntryHi, EntryLo0/1, ASID, V, D e G).
- [x] Operações COP0/TLB experimentais ligadas ao despachante arquitetural: TLBP, TLBR, TLBWI e TLBWR.
- [x] PageMask variável com sete codificações de tamanho de página aceitas pela implementação experimental e máscaras inválidas rejeitadas.
- [x] Estado CP0.Context (registrador 4), leitura/escrita parcial e atualização dos campos BadVPN2/VPN2 em falhas TLB; testes adicionados, aguardando validação no CI.
- [x] Exceções TLB distinguem miss real (refill vector) de entrada programada inválida (general vector); teste unitário adicionado, CI pendente.
- [x] Rastreamento experimental de delay slots no step EE: branch PC, destino após slot, EPC e Cause.BD em falhas de fetch/dados; teste de falha de load no slot e alvo do branch adicionados. CI pendente.
- [x] Regressão do caminho feliz de branch tomado: uma instrução no delay slot, chegada ao alvo e execução da instrução-alvo; `make check` aprovado no CI do commit `a70db3a`.
- [x] Testes host para TLB pareada, ASID, permissões, busca sem correspondência, round-trip CP0/TLB, TLBWR e PageMask.
- [x] Regressão host `make check` aprovada no CI do commit `933a44c` (run `43626390249`).
- [ ] Auditoria diferencial completa da semântica R5900, incluindo PageMask, PFN, delay slots e exceções.
- [ ] Confirmar `make check` no CI após a serialização portátil v2; a última consulta de status não apresentou verificações.
- [x] Snapshot portátil experimental v2: estado EE + TLB arquitetural de 48 entradas + RAM serializados campo a campo em little-endian, cabeçalho versionado, tamanho e checksum; validação do TLB antes da restauração e testes de round-trip/corrupção/wire format adicionados. Execução desses novos testes no CI ainda pendente; exige RAM de mesmo tamanho e não inclui IOP/DMA/VU/GS nem persistência integrada ao runtime.
- [ ] Integração de todas as instruções de memória e fetch no caminho principal da CPU.
- [x] Modelo inicial de biblioteca de jogos: cadastro limitado, proteção contra serial duplicado, favoritos e filtros por título/serial/gênero.
- [x] Snapshot portátil experimental da biblioteca: formato versionado, campos em ordem fixa little-endian, checksum, validação de tamanho/strings/favoritos/seriais duplicados e validação antes de modificar o destino; regressões de round-trip e corrupção adicionadas (CI pendente).
- [ ] Toolchain Xenon configurada e `default.xex` compilado.
- [x] Bloqueios técnicos e ordem de resolução documentados em `docs/KNOWN_BLOCKERS.md`.
- [ ] Inicialização comprovada em Xbox 360 RGH/JTAG.
- [ ] Emulação de jogos PS2.

**Importante:** os módulos EE importados são protótipos experimentais. O bootstrap atual não inicializa o hardware do Xbox 360 nem inicia jogos PS2. Não há ainda um `default.xex` compilado ou testado.

## Módulos EE importados

Os arquivos em `src/ps2/` foram copiados do repositório [PS2Palace360](https://github.com/MeiKinosaki/PS2Palace360), preservando a licença MIT e sua atribuição. Incluem estado EE, operações escalares, imediatas, comparações, decodificação parcial e memória/TLB experimental.

## Build

Validação nativa:

```sh
make check
```

Build final para Xbox 360, em runner que tenha `xenon-gcc` e `elf2xex`:

```sh
make clean
make xex
```

Saída esperada: `build/default.xex`.

O job Xenon do GitHub Actions usa runner self-hosted com labels `linux` e `xenon`; um runner Ubuntu hospedado comum não inclui libXenon automaticamente.

## Próximas etapas

1. Validar no CI o novo modelo de biblioteca e o snapshot EE; evoluir o save/load para formato portátil incluindo TLB/RAM/dispositivos e integrar no runtime. Consultar `docs/KNOWN_BLOCKERS.md` para bloqueios e critérios de conclusão.
2. Fechar o caminho de fetch, load/store e exceções com a TLB arquitetural no executor principal.
3. Integrar scheduler EE, IOP e os dez canais DMA reais.
4. Desenvolver VU0/VU1, VIF, GIF, GS, SPU2 e CDVD.
5. Criar backend gráfico Xenos e runtime Xbox 360.
6. Configurar runner Xenon, gerar `build/default.xex` e testar no console.

## Regras de progresso

Código importado não significa integração completa. Testes importados não significam aprovação no novo repositório. Build host, build Xenon, geração do XEX e execução no console são marcos separados.
