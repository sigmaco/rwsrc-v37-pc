# RenderWare "Graphics" 3 (3.7.0.2), Windows-PC — Substantial Leak

Um vazamento de porções substanciais do código-origem do middleware RenderWare (Graphics).

Como supracitado, é um vazamento de "porções", o código não está completo. Assim sendo, temos aqui:
* O núcleo, que provém abstração básica de elementos de sistemas operacionais, tais como gestão de memória, gestão de arquivos, etc. Também inclue os mecanismos de renderização de modo imediato via Direct3D 8 e 9.
* O plugin RpWorld, que oferce um modo retido de renderização junta a arquitetura DAG para construção de cena. O modo retido opera via Direct3D 8 e 9.
* O plugin RpCollis, que oferece detecção e resolução medíocres de colição. No entanto, ao que eu testei, este em código-origem não opera como a versão original da Criterion, então use a biblioteca pré-compilada.
* O plugin RpToon, que oferece uma técnica de renderização voltada à cartoonização e acentuação de silhueta. Não testei, mas é o que foi usado no game Dragon Ball Online.

~Depois eu complemento isto aqui. Eu tenho mais o que fazer.~

Em caso de dúvida, use nosso suporte no Discord.

[![](https://discordapp.com/api/guilds/349379672351571969/embed.png?style=banner1)](https://discord.gg/vUnjgYD)

Este conteúdo é © 1993-2005 Criterion Software Limited. Todos os direitos reservados. Violação de copyright não-intencional. Isto está aqui para propósitos educativos e preservação digital.
