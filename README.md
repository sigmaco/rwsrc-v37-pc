# RenderWare "Graphics" 3 (3.7.0.2), Windows-PC
Uma distro da versão 3.7.0.2 do SDK de [RenderWare](https://sigmaco.org/t/renderware) Graphics, um [middleware](https://sigmaco.org/t/middleware) adjunto de plugins.
Atualmente, este repositório compreende uma porção substancial em código-origem compilável e executável em [C](https://sigmaco.org/t/c).

Como mencionado acima, isto se trata de uma porção substancial, não do código completo. Aqui temos:
* O core, que provém abstração básica de elementos de sistema operacional, tais como gerenciamento de memória, gerenciamento de arquivo, etc. Também inclui mecanismos para rendição imediata de desenho via [Direct3D](https://sigmaco.org/t/direct3d) 8 e 9.
* O plugin RpWorld, que oferece mecanismos de rendição retida de desenho, juntamente a uma arquitetura para DAGs para edificação de cenários. A rendição retida de desenho opera também via [Direct3D](https://sigmaco.org/t/direct3d) 8 e 9.
* O plugin RpCollis, que oferece detecção e resolução medíocres de colisão. Porém, do que testamos, há inconsistências neste código que não operam como a versão pré-compilada da [Criterion Software](https://sigmaco.org/t/criterion-software).
* O plugin RpToon, que oferece uma técnica de rendição voltada a acentuação de silhueta e cartoonização. Não testamos propriamente este plugin, mas é o que foi usado no gameware Dragon Ball Online.

Para quaisquer dúvidas ou dificuldades, use nosso canal de suporte no [Discord](https://sigmaco.org/t/discord):

[![](https://discordapp.com/api/guilds/349379672351571969/embed.png?style=banner4)](https://sigmaco.org/discord/)

A disponibilização deste repositório é uma resposta do cyber-consórcio brasileiro [SIGMA Technology Group](https://sigmaco.org/g/technology) a solicitações de organizações cibernéticas de modificação de [gamewares](https://sigmaco.org/t/gameware) baseados em RendeWare.
Esta disponibilização está restrita a fins de estudo e modificação de engenharia de gameware. A Criterion Software reserva para si o licenciamento comercial.

[RenderWare](https://sigmaco.org/t/renderware) é © [1993](https://sigmaco.org/t/1993)-[2006](https://sigmaco.org/t/2006) [Criterion Software Ltd.](https://sigmaco.org/t/criterion-software), [Canon Inc.](https://sigmaco.org/t/canon), & [Electronic Arts Inc](https://sigmaco.org/t/ea-games). Todos os direitos reservados.
Contato administrativo: [technology@sigmaco.org](mailto:technology@sigmaco.org).
