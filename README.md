# PCSX2-360

Projeto experimental de emulação de PlayStation 2 para Xbox 360 RGH/JTAG.

**Versão de desenvolvimento: v0.1b — em andamento.**


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
- [x] Operações experimentais equivalentes a TLBP (busca) e TLBR (leitura indexada), além da escrita indexada equivalente a TLBWI.
- [x] Testes host para páginas pares/ímpares, permissões, ASID, entradas duplicadas e round-trip dos registradores TLB; `make check` aprovado no CI do commit `0c6f871`.
- [ ] Integração das operações TLB com registradores CP0 e implementação TLBWR.
- [ ] PageMask variável e semântica completa de exceções/delay slots.
- [ ] Toolchain Xenon configurada e `default.xex` compilado.
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

1. Confirmar a regressão host.
2. Integrar TLBP/TLBR/TLBWI ao banco CP0 e implementar TLBWR, PageMask e semântica arquitetural completa.
3. Integrar scheduler, IOP e DMA real.
4. Desenvolver VU0/VU1, GIF, GS, SPU2 e CDVD.
5. Criar backend gráfico Xenos e runtime Xbox 360.
6. Gerar e testar `default.xex` no console.

## Regras de progresso

Código importado não significa integração completa. Testes importados não significam aprovação no novo repositório. Build host, build Xenon, geração do XEX e execução no console são marcos separados.
