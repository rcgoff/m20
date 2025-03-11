<h1 align="center">Digital Computer M-20 Emulator</h1>
<p align="center">Accurate emulator for soviet general-purpose mainframe M-20</p>
<p align="center">
    <a href="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml"><img src="https://github.com/rcgoff/m20/actions/workflows/m20-ci.yml/badge.svg" alt="M-20 BUILD"></a>
    <a href="LICENSE"><img src="https://img.shields.io/github/license/rcgoff/m20.svg" alt="License"></a> 
</p>

Languages: [:ru: Русский](./Readme.rus.md) | [:us: English](./Readme.md)

## Building

<table>
  <tr>
    <th></th>
    <th>Linux</th>
    <th>Windows <br>(MS Visual Studio C)</th>
    <th>Windows<br>(MinGW32)</th>
    <th>Windows<br>(MinGW64)</th>
  </tr>
  <tr>
    <td>Go to the sources directory:</td>
    <td colspan="4"><pre><code class="language-shell">cd sources/emulator</code></pre></td>
   </tr>
   <tr>
     <td>Build project:</td>
     <td><pre><code class="language-bash">make -f makefile.unx</code></pre> </td>
     <td><pre><code class="language-batch">build_win</code></pre></td>
     <td><pre><code class="language-batch"> build_mingw32</code></pre></td>
     <td><pre><code class="language-batch">build_mingw64</code></pre></td>
   </tr>
   <tr>
     <td>Run tests:</td>
     <td><pre><code class="language-bash">make -f makefile.unx test</code></pre> </td>
     <td><pre><code class="language-batch">build_win test</code></pre></td>
     <td><pre><code class="language-batch"> build_mingw32 test</code></pre></td>
     <td><pre><code class="language-batch">build_mingw64 test</code></pre></td>
   </tr>
   <tr>
     <td>Clean project:</td>
     <td><pre><code class="language-bash">make -f makefile.unx clean </code></pre></td>
     <td colspan="3"><pre><code class="language-bash">clean_win </code></pre></td>
   </tr>
</table>

