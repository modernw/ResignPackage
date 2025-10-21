(Translated by CharGTP)
<h1>如何给包进行签名<br>How to Sign an App Package</h1>

<h2>创建签名文件<br>Create a Signature File</h2>

<h3>准备文件<br>Prepare Required Files</h3>
<p>以下文件来自 Windows Kits（包括 Windows 8.x 和 10），如果进行 Metro/UWP 应用开发，通常已安装这些工具。</p>
<p>The following files come from the Windows Kits (Windows 8.x or 10). If you have developed Metro/UWP apps, you likely already have them.</p>

<ul>
  <li>makecert.exe – 签名创建工具 / Certificate creation tool</li>
  <li>pvk2pfx.exe – 将 PVK 文件转换为 PFX 文件 / Converts PVK to PFX file</li>
</ul>

<p>除了从 Windows Kits 获取这些文件，还可以从 <b>WSAppBak</b> 获取（该项目附带这些文件，用于 Appx 打包和签名）。</p>
<p>You can also obtain them from <b>WSAppBak</b> (it includes all tools needed for Appx packaging and signing).</p>

---

<h3>创建 CER 和 PVK 文件<br>Create CER and PVK Files</h3>
<p>该教程来自 WSAppBak 的源码。这里只介绍操作步骤，不涉及原理。</p>
<p>This tutorial is based on the WSAppBak source code. Here we only describe the steps, not the underlying principles.</p>

<p>打开命令行，为了方便操作，先切换到包含这些工具的目录。</p>
<p>Open Command Prompt and navigate to the folder where the tools are located.</p>

<p>输入以下命令：</p>
<p>Enter the following command:</p>

<pre><code>makecert.exe -n &lt;Identity_Publisher&gt; -r -a sha256 -len 2048 -cy end -h 0 -eku 1.3.6.1.5.5.7.3.3 -b 01/01/2000 -sv &lt;Output_PVK_File_Path&gt; &lt;Output_CER_File_Path&gt;</code></pre>

<p>例如：</p>
<p>Example:</p>

<pre><code>"E:\Profiles\Bruce\Desktop\Others\wsapp\Build\MakeCert.exe" -n "CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US" -r -a sha256 -len 2048 -cy end -h 0 -eku 1.3.6.1.5.5.7.3.3 -b 01/01/2000 -sv "Output\Microsoft.3DBuilder_x86.pvk" "Output\Microsoft.3DBuilder_x86.cer"</code></pre>

<p>执行后会弹出一个窗口（截图中文本为翻译版，原为英文），提示是否创建密码。根据需要选择是否设置密码。密码不会影响 CER 证书的导入，但会影响 PFX 文件的导入。建议直接点击 “OK” 或 “None”，不要直接关闭窗口。</p>

<p>After execution, a dialog will appear (text shown here is translated from English). It asks whether to set a password. You may skip it — the password does not affect CER import but is required for PFX import. Click “OK” or “None”; do not close the window directly.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-1.png">

<p>如图所示，如果创建失败，会输出错误信息；如果成功，会显示 “Succeeded”。</p>
<p>If it fails, an error message will appear; on success, you’ll see “Succeeded”.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-2.png">
<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-2-3.png">

---

<h3>创建 PFX 文件<br>Create a PFX File</h3>

<p>该教程同样来自 WSAppBak 源码，只介绍操作方法。</p>
<p>This tutorial also comes from WSAppBak; we focus on the practical method only.</p>

<p>打开命令行并定位到工具所在目录，准备好刚才生成的 CER 和 PVK 文件（这两个文件必须配套）。</p>
<p>Open Command Prompt, navigate to the tool’s directory, and prepare the previously created CER and PVK files (they must match).</p>

<p>输入以下命令：</p>
<p>Enter the following command:</p>

<pre><code>pvk2pfx.exe -pvk &lt;PVK_File_Path&gt; -spc &lt;CER_File_Path&gt; -pfx &lt;Output_PFX_File_Path&gt;</code></pre>

<p>例如：</p>
<p>Example:</p>

<pre><code>"E:\Profiles\Bruce\Desktop\Others\wsapp\Build\Pvk2Pfx.exe" -pvk "Output\Microsoft.3DBuilder_x86.pvk" -spc "Output\Microsoft.3DBuilder_x86.cer" -pfx "Output\Microsoft.3DBuilder_x86.pfx"</code></pre>

<p>如果成功，命令行不会输出任何内容。</p>
<p>If successful, no message will be displayed.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/makesign-3-1.png">

<p>如果创建 CER/PVK 时设置了密码，生成 PFX 时需再次输入该密码。成功时同样不会有输出。</p>
<p>If you set a password when creating the CER/PVK files, you’ll need to enter it again here. Successful execution also produces no output.</p>

<p>这里的窗口是我自行翻译的，实际版本中通常为英文界面。</p>
<p>The screenshots are localized; the actual dialogs are in English.</p>

<p>PFX 文件用于导入签名或为应用商店包重新签名。CER 文件用于导入证书，而 CER + PVK 则用于生成 PFX 文件。</p>
<p>The PFX file is used for signature import or re-signing Store packages. The CER file is for certificate import, and CER + PVK together generate the PFX file.</p>

---

<h2>包的签名<br>Signing the Package</h2>

<h3>准备文件<br>Prepare Required Files</h3>
<p>以下文件同样来自 Windows Kits（Windows 8.x 或 10）。</p>
<p>The following file also comes from the Windows Kits (Windows 8.x or 10).</p>

<ul><li>signtool.exe – 应用包签名工具 / Signing tool for App packages</li></ul>

<p>建议右击查看程序版本号，推荐使用 6.3.xxxxx.xxxxx 或更高版本。6.2.xxxxx.xxxxx 不支持签名 AppxBundle。至于 Msix/MsixBundle 签名，不在本篇范围内。</p>
<p>Check the version before use. Recommended version: 6.3.xxxxx.xxxxx or higher. Version 6.2.xxxxx.xxxxx does not support AppxBundle signing. Msix/MsixBundle signing is not covered here.</p>

<p>这些文件同样可从 WSAppBak 项目中获得。</p>
<p>You can also get them from the WSAppBak project.</p>

---

<h3>使用 SignTool 对包进行签名<br>Sign Packages with SignTool</h3>

<p>该教程来自 WSAppBak 的源码，这里只介绍操作步骤。</p>
<p>This guide comes from WSAppBak’s source code — only the steps are shown here.</p>

<p>准备一个应用包（Appx/AppxBundle/Msix/MsixBundle，确保 SignTool 支持），以及 PFX 签名文件。</p>
<p>Prepare an app package (Appx/AppxBundle/Msix/MsixBundle — make sure SignTool supports it) and a PFX signature file.</p>

<p>打开命令行，定位到 signtool.exe 所在目录。</p>
<p>Open Command Prompt and navigate to the folder containing signtool.exe.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-1.png">

<p>输入以下命令：</p>
<p>Enter the following command:</p>

<pre><code>signtool.exe sign -fd SHA256 -a -f &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code></pre>

<p>例如：</p>
<p>Example:</p>

<pre><code>signtool.exe sign -fd SHA256 -a -f "Sign File.pfx" "File WillSign.appx"</code></pre>

<p>等待命令完成，当输出 “Successfully signed” 时表示签名成功。</p>
<p>Wait for the command to finish. If you see “Successfully signed,” the signing is complete.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-3.png">

---

<h3>简化操作<br>Automation Tip</h3>

<p>可以通过创建快捷方式简化命令行签名操作。</p>
<p>You can simplify signing operations by using a shortcut.</p>

<p>例如，将一个快捷方式的目标设为：<br>
<code>"E:\Profiles\Bruce\Desktop\WSAppBak 1.1\WSAppBak\signtool.exe" sign -fd SHA256 -a -f</code><br>
拖入文件时执行命令行：<br>
<code>signtool.lnk &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code><br>
其效果等同于：<br>
<code>&lt;signtool_path&gt; sign -fd SHA256 -a -f &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code></p>

<p>For example, create a shortcut with this target:<br>
<code>"E:\Profiles\Bruce\Desktop\WSAppBak 1.1\WSAppBak\signtool.exe" sign -fd SHA256 -a -f</code><br>
Then drag a file onto the shortcut to execute:<br>
<code>signtool.lnk &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code><br>
This effectively runs:<br>
<code>&lt;signtool_path&gt; sign -fd SHA256 -a -f &lt;PFX_File_Path&gt; &lt;Package_File_Path&gt;</code></p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/lnkcmd.png">

---

<h3>注意事项<br>Notes</h3>

<p>签名 AppxBundle 时，请使用较新版本的 signtool.exe（建议使用 Windows 8.1 的 Kits）。旧版本会输出错误。</p>
<p>When signing AppxBundles, use a newer signtool.exe (e.g., from Windows 8.1 Kits). Older versions will fail with an error message.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-6.png">

<p>注意证书颁发者信息。如果签名证书的 Publisher 与包清单中的 Identity Publisher 不一致，签名可能会失败。</p>
<p>Ensure the certificate’s Publisher matches the Identity Publisher in the package manifest, or signing may fail.</p>

<img class="domain-img domain-img-center" src="https://modernw.github.io/MetroTipsOnline/Articles/MakePackage/Images/signpkg-2-7.png">

<p>通常无需对商店应用的运行时包或 AppxBundle 中的资源包进行重签名。重签名仅适用于安装后可直接运行的应用包。</p>
<p>Typically, you don’t need to re-sign runtime or resource packages inside AppxBundles. Re-signing applies only to installable and runnable application packages.</p>

<p>如果包因数字签名问题导致无法安装，这时才需要考虑重新签名。</p>
<p>Only consider re-signing when installation fails due to signature issues.</p>

<p>运行时包和资源包（一般为 Appx/Msix 格式）的清单文件（AppxManifest.xml）中通常没有 <code>&lt;Applications&gt;</code> 节点，或其下没有 <code>&lt;Application&gt;</code> 子节点及有效的 Id 属性。这类包安装后仅用于资源调用。</p>

<p>Runtime and resource packages (usually in Appx/Msix format) typically lack the <code>&lt;Applications&gt;</code> node, or it contains no valid <code>Id</code> attribute. Such packages are used only for resource access, not for execution after installation.</p>
