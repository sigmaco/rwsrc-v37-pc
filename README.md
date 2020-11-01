# RenderWare "Graphics" 3 (3.7.0.2), Windows-PC — Substantial Leak
A leak of substantial portions of source-code of RenderWare Graphics 3.7.0.2 for Windows-PC.
For more about [RenderWare](https://sigmaco.org/?s=renderware&bp_search=1&view=content), look at [SIGMAco.org](https://sigmaco.org), the our brazilian hub.

As mentioned above, it is a leak of "portions", the code is not complete. So here we have it:
* The core, which provides basic abstraction of elements of operating systems, such as memory management, file management, etc. It also includes mechanisms for immediate draw rendering via [Direct3D](https://sigmaco.org/p/tag/direct3d) 8 and 9.
* The RpWorld plugin, which offers a retained rendering mode, joins the DAG architecture for scene construction. The retained mode operates via [Direct3D](https://sigmaco.org/p/tag/direct3d) 8 and 9.
* The RpCollis plugin, which offers mediocre collision detection and resolution. However, from what I tested, this source code does not operate like the original version of [Criterion Software](https://sigmaco.org/p/tag/criterion-software), so you should use the precompiled library.
* The RpToon plugin, which offers a rendering technique aimed at cartoonization and silhouette accentuation. I haven't tested it, but it's what was used in the Dragon Ball Online game.
~There are many insights to write there...~

For any questions, use our support at Discord:

[![](https://discordapp.com/api/guilds/349379672351571969/embed.png?style=banner1)](https://discord.gg/vUnjgYD?SIGMA_Technology_Group)

[RenderWare](https://sigmaco.org/?s=renderware&bp_search=1&view=content) is © [1993](https://sigmaco.org/p/tag/1993)-[2006](https://sigmaco.org/p/tag/2006) [Criterion Software Ltd.](https://sigmaco.org/p/tag/criterion-software), [Canon Inc.](https://sigmaco.org/p/tag/canon), & [Electronic Arts Inc](https://sigmaco.org/p/tag/ea-games). All rights reserved.
This repository is intended merely to preserve the history and legacy of [RenderWare](https://sigmaco.org/?s=renderware&bp_search=1&view=content). No copyright infringement targets any harm to its legal holders. For any objections, contact the [SIGMA Technology Group](https://sigmaco.org/g/technology) at [technology@sigmaco.org](mailto:technology@sigmaco.org).
