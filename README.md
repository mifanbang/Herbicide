# Herbicide

Removing flowers and rabbits in the game Mirror<br>
消除 Mirror 這 ê 遊戲內面 ê 花仔 kap 兔仔 ê ke-si<br>
用来消除 Mirro 这款游戏内的小花和兔子的工具

## Introduction 介紹 简介

The gameplay of Mirror is very obscene by its nature. Thus censorship for insignificant things, e.g., the nipples, is clearly nonsense. The artist(s) actually had done a great job painting those amazing illustrations and you don't want to miss any detail in the game.

Mirror ê gameplay 有足濟有影真害 ê mi̍h-kiānn，所以遊戲設計者 ka 少寡其實一寡仔攏 bô 重要 ê 部份 am 起來（譬如講奶頭）實在是完全 bô 道理 ê。而且畫家足認真畫 ê 圖 na 是 bô 法度予玩家體驗，講起來不但足可惜，猶 ko 誠對不起付錢 ê 消費者。

Mirror 在先天上就是一款被设计成拥有着高度糟糕 gameplay 的游戏，这时候如果还在些完全不打紧的关头上搞河蟹，实在不是一件有什麽道理的事儿。况且想那绘师如此认真的画图，不让玩家好好体验其作品美妙之处的话，未免太可惜也太对不起付了钱的广大消费者。

## Disclaimers 免責放聲 免责声明

In addition to the lack of warranty as this software is distributed, the use of this software may lead to violation of an agreement that you have agreed with Valve Corporation and/or any other party regarding the game [*Mirror*](https://store.steampowered.com/app/644560/Mirror/). Please be aware of that you are totally at your own risk.

這 ê 軟體除了 bô 欲提供任何使用上 ê 保固以外，lí 佇使用時猶 ko 有可能會反背當初 kap Valve Coporation 公司抑 ko 是 kap 任何 [*Mirror*](https://store.steampowered.com/app/644560/Mirror/) ê 權利所有者之間所簽 ê 合約。請注意 lí 必須為使用這 ê 軟體 ê 所有個人行為負責。

这款软件除了不提供任何使用上的保证之外，在使用上将可能导致您违反与 Valve Coporation 公司或与任何其他拥有 [*Mirror*](https://store.steampowered.com/app/644560/Mirror/) 智慧财产权的个人或团体之间的合同。请留意您必须在使用本软件时为个人行为完全负责。

## How to Use 使用方法 使用方式

Just run the program and stay calm.

只要執行這 ê 軟體 tō 夠啊。少年 ê，ái 記得保持冷靜 hâ！

玩家只需要下载并执行此一软件即可。

## Known Issues 既明問題 已知问题

Since the project cannot contain any copyrighted material from the game, it can only unsensor if the censorship is done by overlaying multiple game assets. Certain contents of the game, e.g., the ending scenes, may not be uncensored because the censorship is built into the illustration as integral part of a single asset.

因為這 ê 專案 bē-sái 使用遊戲內底受 tò 著作權保護 ê 資料，它 kan-na 有法度處理用超過一 ê 圖層合成來進行 censorship ê 所在。譬如講每一 ê 角色結局劇情，因為 censorship 是 hit 款 ê 圖當中 ê 一部分，án-ne 這款軟體所使用的方法 tō bô-khó-lîng 會當予伊藏起來。

这个项目由于无法使用受到了著作权保护的各种游戏内资料，因此只能针对透过多图层相叠实现和谐的游戏部份进行下马。对于那些在单张图像内就已被直接河蟹掉的部分（例如各角色的结局剧情），很遗憾地本软件可说是束手无策。

## Building the Code 建構程式 代码建置

The solution file and project files in this repository are maintained with [Visual Studio 2017](https://www.visualstudio.com/vs/community/). Support of various features in C++17 and the function attribute `__declspec(naked)` are required if you are building with other compilers. Please also note that Herbicide uses DLL injection as its core mechanism and Mirror is still built as 32-bit application, so there's no need to build a 64-bit version of the project.

所有的程式碼 kap 專案檔案攏是使用 [Visual Studio 2017](https://www.visualstudio.com/vs/community/) 來進行維護。Na 是愛用別 ê compiler 進行建構，少寡 C++17 ê 功能 kap 函數屬性 `__declspec(naked)` 必須愛予伊支援 tsiah 會使。另外因為 Mirror 猶 ko 是 32-bit ê 程式，而且 Herbicide 主要用 DLL injection ê 方法運作，所以 bô 需要將伊建構成 64-bit 程式。

所有的代码与项目都透过 [Visual Studio 2017](https://www.visualstudio.com/vs/community/) 进行维持。由于代码中使用了若干 C++ 17 功能以及 `__declspec(naked)` 函数属性，若使用其他编译器进行建置时须留意这些特性是否获得工具的支持。除此之外，基于 Mirror 仍然是 32 位应用并且 Herbicide 使用 DLL 注入的机制运行，因此并不需要另外为本项目建置 64 位的版本。

## Notes on Anti-Virus 關於防毒軟體 关于杀毒软件

Some ill-developed anti-virus software may report Herbicide as malware. Please do not panic. You can either add Herbicide into the exception list of your AV (if you trust this software) or submit a report in the [Issues page](https://github.com/mifanbang/Herbicide/issues).

有一寡設計 ê bô 啥好 ê 防毒軟體可能會 ka Herbicide 當作歹意軟體。Tú-tio̍h 這款情形 ê 時陣請 mài 緊張，會當將這 ê 程式加入例外清單內。Na 是有任何相關的問題，攏 mā 歡迎佇 [Issues page](https://github.com/mifanbang/Herbicide/issues) 底提出。

有些设计不良的杀毒软件可能会将 Herbicide 视为恶意软件，如果遇到了这种情况请无需感到紧张。通常只需要将本软件加入杀毒的例外清单内即可。若有任何相关的问题，都欢迎于 [Issues page](https://github.com/mifanbang/Herbicide/issues) 页面提出。

## Copyright 著作權資訊 著作权信息

Copyright (C) 2018 Mifan Bang <https://debug.tw>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
