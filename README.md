<h1>如何给包进行签名</h1>
<h2>创建签名文件</h2>
<h3>准备文件</h3>
以下文件来自 Windows Kits（包括 Windows 8.x 和 10），如果做 Metro/UWP 应用的相关开发会有这些工具。
<ul>
  <li>makecert.exe – 签名创建工具</li>
  <li>pvk2pfx.exe – 将 PVK 文件转换为 PFX 文件</li>
</ul>

除了从 Windows Kits 获取以上文件，还可以从 WSAppBak 获取。（WSAppBak 会带这些文件，用于 Appx 的打包和签名）

<h3>创建 CER 和 PVK 文件</h3>
该教程来自 WSAppBak 的源码。这里不介绍原理，只介绍方法，具体的上网搜。
打开命令行，为了方便操作，我们直接 dir 到前文提到的工具所在的目录。

我们输入以下命令

<pre><code>makecert.exe -n &lt;Identity_Publisher&gt; -r -a sha256 -len 2048 -cy end -h 0 -eku 1.3.6.1.5.5.7.3.3 -b 01/01/2000 -sv &lt;Output_PVK_File_Path&gt; &lt;Output_CER_File_Path&gt;</code></pre>

例如：

<pre><code>"E:\Profiles\Bruce\Desktop\Others\wsapp\Build\MakeCert.exe" -n "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US" -r -a sha256 -len 2048 -cy end -h 0 -eku 1.3.6.1.5.5.7.3.3 -b 01/01/2000 -sv "Output\Microsoft.3DBuilder_x86.pvk" "Output\Microsoft.3DBuilder_x86.cer"</code></pre>

如截图所示，这时候弹出一个窗口（窗口上的文本我自己用 Resource Hacker 翻译了，实际上都是英文），提示是否创建密码。这个根据自己的需求。密码不影响 CER 证书导入，影响 PFX 证书的导入。这里我们就不用密码了。（注意，不要直接关闭窗口，而是点“OK”或“None”

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-1.png">

如截图所示，如果没有创建成功，则会输出错误信息。如果成功，会输出“Succeeded”

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-2.png">
<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-3.png">

<h3>创建 PFX 文件</h3>

该教程来自 WSAppBak 的源码。这里不介绍原理，只介绍方法，具体的上网搜。

打开命令行，为了方便操作，我们直接 dir 到前文提到的工具所在的目录。

准备之前的 CER 文件和 PVK 文件（这两个文件是配套的）

输入以下命令行：

<pre><code class="text-code-font">pvk2pfx.exe -pvk &lt;PVK_File_Path&gt; -spc &lt;CER_File_Path&gt; -pfx &lt;Output_PFX_File_Path&gt;</code></pre>

例如：

<pre><code class="text-code-font">"E:\Profiles\Bruce\Desktop\Others\wsapp\Build\Pvk2Pfx.exe" -pvk "Output\Microsoft.3DBuilder_x86.pvk" -spc "Output\Microsoft.3DBuilder_x86.cer" -pfx "Output\Microsoft.3DBuilder_x86.pfx"</code></pre>

如果成功了，那么不会有什么输出

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-3-1.png">

如果在创建 CER 文件和 PVK 文件使用密码，那么在创建 PFX 文件时仍要输入密码。（与创建 CER 文件和 PVK 文件输入的密码一样）。同样，成功是不会有什么输出


这里的窗口仍然是我翻译并修改的，其实平常能获得到的资源是英文。

PFX 将用来导入签名，或者用于应用商店包的重签名。

CER 将用于导入证书。

CER 和 PVK 用于生成 PFX 文件。

<h2>包的签名</h2>
<h3>准备文件</h3>
以下文件来自 Windows Kits（包括 Windows 8.x 和 10），如果做 Metro/UWP 应用的相关开发会有这些工具。

<ul><li>signtool.exe – 应用包签名工具。</li></ul>

注意，使用前请建议右击查看程序的版本，建议版本为 6.3.xxxxx.xxxxx 版本及以上，6.2.xxxxx.xxxxx 版本不支持签名 AppxBundle 包。至于 Msix/MsixBundle 包的签名，这里不做考虑。（看 Msix/MsixBundle 是从哪个系统版本出现就从哪个系统版本的 Kits 提取）

除了从 Windows Kits 获取以上文件，还可以从 WSAppBak 获取。（WSAppBak 会带这些文件，用于 Appx 的打包和签名）

<h3>使用 SignTool 对包进行签名</h3>
该教程来自 WSAppBak 的源码。这里不介绍原理，只介绍方法，具体的上网搜。

准备一个应用商店包（Appx/AppxBundle/Msix/MsixBundle，确保是 SignTool 支持的），和 PFX 签名文件。

打开命令行，为了方便操作，我们直接 dir 到 signtool.exe 所在的目录。

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-1.png">

然后我们输入以下命令行：

<pre><code class="text-code-font">signtool.exe sign -fd SHA256 -a –f &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code></pre>

例如：

<pre><code class="text-code-font">signtool.exe sign -fd SHA256 -a -f "Sign File.pfx" "File WillSign.appx"</code></pre>

等待完成签名，当出现“Successfully signed”就意味着签名成功。

如截图：

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-3.png">

<p class="text-code-font">简化操作：<br>我们不用每次都输入那几个不变的命令参数，只需要创建一个快捷方式即可。<br>我们把一个文件拖到快捷方式，执行的是“exec.lnk %1”（%1 作为占位符指的是传入的命令行参数文本，这里指的是拖入的文件路径文本，懂相关编程的应该会了解），假设快捷方式的目标为“exec.exe”，那么执行的是“exec.exe %1”<br>我们创建一个 signtool 的快捷方式，把目标设为“&lt;signtool_file_path&gt; sign -fd SHA256 -a -f”，如“"E:\Profiles\Bruce\Desktop\WSAppBak 1.1\WSAppBak\signtool.exe" sign -fd SHA256 -a -f”，当我们用命令行调用这个快捷方式时，直接输入：<br>signtool.lnk &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;<br>实际上执行的是“&lt;signtool_file_path&gt; sign -fd SHA256 -a –f &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;”</p>

如截图：

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/lnkcmd.png">

注意：
签名 AppxBundle 时，请用较高版本的 signtool.exe，AppxBundle 在 Windows 8.1 出现，那么用 8.1 的 Windows Kits 即可。如果 signtool.exe 版本不支持，会出现以下输出。

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-6.png">

注意证书颁发者（我也没弄清具体原因），如果与商店应用包的清单中的 Identity Publisher 不对应，可能会导致签名失败

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-7.png">

通常我们不用对商店应用的运行时包和 AppxBundle 中的资源包进行重签名。重签名适用于安装后能运行应用的包。运行时包和资源包并不属于这种安装后能运行应用的包。

还有一种情况是这个包因为数字签名而无法安装，那么我们就要考虑重签名了。

运行时包和资源包（一般是 Appx/Msix 格式）的清单文件（AppxManifest.xml）中没有“Applications”节点或者“Applications”节点中无”Application”子节点或”Application”子节点中无“Id”属性值或“Id”属性值中文本为空



运行时包和资源包并不属于这种安装后能运行应用的包，因为这种包安装后仅会用于调用资源。可以从清单文件中看出，连“Applications”这个节点都没有


