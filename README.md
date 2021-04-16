# RenderWare "Graphics" 3 (3.7.0.2), Windows-PC — Substantial Leak
A leak of substantial portions of source-code of [RenderWare](https://sigmaco.org/t/renderware) Graphics 3.7.0.2 for Windows-PC.

As mentioned above, it is a leak of "portions", the code is not complete, so here we have:
* The core, which provides basic abstraction of elements of operating systems, such as memory management, file management, etc. It also includes mechanisms for immediate draw rendering via [Direct3D](https://sigmaco.org/t/direct3d) 8 and 9.
* The RpWorld plugin, which offers a retained rendering mode, joins the DAG architecture for scene construction. The retained mode operates via [Direct3D](https://sigmaco.org/t/direct3d) 8 and 9.
* The RpCollis plugin, which offers mediocre collision detection and resolution. However, from what I tested, this source code does not operate like the original version of [Criterion Software](https://sigmaco.org/t/criterion-software), so you should use the precompiled library.
* The RpToon plugin, which offers a rendering technique aimed at cartoonization and silhouette accentuation. I haven't tested it, but it's what has been used in the Dragon Ball Online game.
~There are many insights to write there...~

For any questions, use our support at [Discord](https://sigmaco.org/discord/):

[![](https://discordapp.com/api/guilds/349379672351571969/embed.png?style=banner4)](https://sigmaco.org/discord/)

[RenderWare](https://sigmaco.org/t/renderware) is © [1993](https://sigmaco.org/t/1993)-[2006](https://sigmaco.org/t/2006) [Criterion Software Ltd.](https://sigmaco.org/t/criterion-software), [Canon Inc.](https://sigmaco.org/t/canon), & [Electronic Arts Inc](https://sigmaco.org/t/ea-games). All rights reserved.
This repository is intended merely to preserve the history and legacy of [RenderWare](https://sigmaco.org/t/renderware). No copyright infringement targets any harm to its legal holders. For any objections, contact the [SIGMA Technology Group](https://sigmaco.org/g/technology) at [technology@sigmaco.org](mailto:technology@sigmaco.org).
