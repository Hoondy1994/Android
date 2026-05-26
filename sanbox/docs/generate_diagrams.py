# -*- coding: utf-8 -*-
"""Generate UTF-8 SVG diagrams with ASCII-only source (avoids encoding corruption)."""
from pathlib import Path

DOCS = Path(__file__).resolve().parent


def arch_svg() -> str:
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1280 920" width="1280" height="920">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#f8f9fc"/>
      <stop offset="100%" stop-color="#eef1f8"/>
    </linearGradient>
    <linearGradient id="uiGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#7c4dff"/>
      <stop offset="100%" stop-color="#6200ee"/>
    </linearGradient>
    <linearGradient id="ctrlGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#536dfe"/>
      <stop offset="100%" stop-color="#3d5afe"/>
    </linearGradient>
    <linearGradient id="bizGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#26a69a"/>
      <stop offset="100%" stop-color="#00897b"/>
    </linearGradient>
    <linearGradient id="jniGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#ffb74d"/>
      <stop offset="100%" stop-color="#fb8c00"/>
    </linearGradient>
    <linearGradient id="nativeGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#ff7043"/>
      <stop offset="100%" stop-color="#e64a19"/>
    </linearGradient>
    <linearGradient id="osGrad" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#455a64"/>
      <stop offset="100%" stop-color="#263238"/>
    </linearGradient>
    <marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 z" fill="#546e7a"/>
    </marker>
    <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="3" stdDeviation="4" flood-color="#000000" flood-opacity="0.12"/>
    </filter>
    <style>
      .title {{ font: 700 28px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#1a237e; }}
      .subtitle {{ font: 400 14px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#546e7a; }}
      .layer-title {{ font: 700 16px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#fff; }}
      .layer-sub {{ font: 400 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:rgba(255,255,255,.92); }}
      .box-title {{ font: 700 14px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#263238; }}
      .box-text {{ font: 400 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#455a64; }}
      .mono {{ font: 400 11px Consolas,monospace; fill:#37474f; }}
      .legend-text {{ font: 400 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#37474f; }}
      .note {{ font: 400 11px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#78909c; }}
    </style>
  </defs>
  <rect width="1280" height="920" fill="url(#bg)"/>
  <text x="640" y="42" text-anchor="middle" class="title">{"\u6c99\u7bb1 Demo \u7cfb\u7edf\u67b6\u6784\u56fe"}</text>
  <text x="640" y="68" text-anchor="middle" class="subtitle">com.example.san | {"\u002b Android \u5e94\u7528\u6c99\u7bb1 + Native Seccomp \u53cc\u5c42\u9694\u79bb\u6f14\u793a"}</text>
  <g filter="url(#shadow)">
    <rect x="490" y="88" width="300" height="52" rx="26" fill="#fff" stroke="#cfd8dc"/>
    <text x="640" y="119" text-anchor="middle" class="box-title">{"\u7528\u6237 / \u6d4b\u8bd5\u4eba\u5458"}</text>
  </g>
  <line x1="640" y1="140" x2="640" y2="162" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <g filter="url(#shadow)">
    <rect x="80" y="162" width="1120" height="108" rx="12" fill="url(#uiGrad)"/>
    <text x="104" y="192" class="layer-title">{"\u8868\u73b0\u5c42 UI"}</text>
    <text x="104" y="212" class="layer-sub">activity_main.xml + ViewBinding</text>
    <rect x="104" y="228" width="170" height="28" rx="6" fill="rgba(255,255,255,.18)"/>
    <text x="189" y="246" text-anchor="middle" class="layer-sub">{"4 \u4e2a\u6f14\u793a\u6309\u94ae"}</text>
    <rect x="290" y="228" width="170" height="28" rx="6" fill="rgba(255,255,255,.18)"/>
    <text x="375" y="246" text-anchor="middle" class="layer-sub">info_text {"\u4fe1\u606f\u5361\u7247"}</text>
    <rect x="476" y="228" width="170" height="28" rx="6" fill="rgba(255,255,255,.18)"/>
    <text x="561" y="246" text-anchor="middle" class="layer-sub">log_scroll {"\u65e5\u5fd7\u533a"}</text>
    <rect x="662" y="228" width="220" height="28" rx="6" fill="rgba(255,255,255,.18)"/>
    <text x="772" y="246" text-anchor="middle" class="layer-sub">btn_clear_log {"\u6e05\u7a7a\u65e5\u5fd7"}</text>
  </g>
  <line x1="640" y1="270" x2="640" y2="292" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <g filter="url(#shadow)">
    <rect x="180" y="292" width="920" height="96" rx="12" fill="url(#ctrlGrad)"/>
    <text x="204" y="322" class="layer-title">{"\u63a7\u5236\u5c42 Controller"}</text>
    <text x="204" y="342" class="layer-sub">MainActivity.kt</text>
    <rect x="204" y="354" width="200" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="304" y="370" text-anchor="middle" class="layer-sub">onCreate() {"\u521d\u59cb\u5316"}</text>
    <rect x="420" y="354" width="200" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="520" y="370" text-anchor="middle" class="layer-sub">runDemo(title, block)</text>
    <rect x="636" y="354" width="200" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="736" y="370" text-anchor="middle" class="layer-sub">appendLog() {"\u8f93\u51fa\u7ed3\u679c"}</text>
    <rect x="852" y="354" width="220" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="962" y="370" text-anchor="middle" class="layer-sub">{"\u6309\u94ae\u8def\u7531\u5230\u5bf9\u5e94 Demo"}</text>
  </g>
  <line x1="420" y1="388" x2="320" y2="430" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <line x1="640" y1="388" x2="640" y2="430" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <line x1="860" y1="388" x2="960" y2="430" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <text x="250" y="422" class="note">Demo {"\u2460\u2461"}</text>
  <text x="610" y="422" class="note">Demo {"\u2462\u2463"} {"\u51c6\u5907\u6587\u4ef6"}</text>
  <text x="890" y="422" class="note">Demo {"\u2462\u2463"}</text>
  <g filter="url(#shadow)">
    <rect x="80" y="430" width="560" height="170" rx="12" fill="url(#bizGrad)"/>
    <text x="104" y="460" class="layer-title">{"\u4e1a\u52a1\u5c42 Android \u6c99\u7bb1\u6f14\u793a"}</text>
    <text x="104" y="480" class="layer-sub">SandboxDemo.kt (Kotlin / JVM)</text>
    <rect x="104" y="494" width="240" height="88" rx="8" fill="#fff" fill-opacity=".95"/>
    <text x="120" y="516" class="box-title">Demo {"\u2460"} demoPrivateFileAccess()</text>
    <text x="120" y="536" class="box-text">{"\u5199\u5165 filesDir/sandbox_secret.txt"}</text>
    <text x="120" y="554" class="box-text">{"\u8bfb\u56de\u5e76\u6821\u9a8c\u79c1\u6709\u76ee\u5f55\u53ef\u8bbf\u95ee"}</text>
    <text x="120" y="572" class="mono">path: context.filesDir</text>
    <rect x="364" y="494" width="250" height="88" rx="8" fill="#fff" fill-opacity=".95"/>
    <text x="380" y="516" class="box-title">Demo {"\u2461"} demoCrossBoundaryRead()</text>
    <text x="380" y="536" class="box-text">canRead() {"\u68c0\u6d4b settings \u76ee\u5f55"}</text>
    <text x="380" y="554" class="box-text">{"\u5bf9\u6bd4\u672c\u5e94\u7528\u76ee\u5f55 UID \u9694\u79bb"}</text>
    <text x="380" y="572" class="mono">/data/data/com.android.settings</text>
  </g>
  <g filter="url(#shadow)">
    <rect x="680" y="430" width="520" height="78" rx="12" fill="url(#jniGrad)"/>
    <text x="704" y="460" class="layer-title">JNI {"\u6865\u63a5\u5c42"}</text>
    <text x="704" y="480" class="layer-sub">SandboxNative.kt + System.loadLibrary("san")</text>
    <rect x="704" y="488" width="220" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="814" y="504" text-anchor="middle" class="layer-sub">runNativeFileRead(path)</text>
    <rect x="944" y="488" width="240" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="1064" y="504" text-anchor="middle" class="layer-sub">runSeccompBlockedRead(path)</text>
  </g>
  <g filter="url(#shadow)">
    <rect x="680" y="522" width="520" height="78" rx="12" fill="url(#nativeGrad)"/>
    <text x="704" y="552" class="layer-title">Native {"\u5c42 libsan.so"}</text>
    <text x="704" y="572" class="layer-sub">native-lib.cpp + CMake</text>
    <rect x="704" y="580" width="220" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="814" y="596" text-anchor="middle" class="layer-sub">readFilePreview() openat</text>
    <rect x="944" y="580" width="240" height="24" rx="5" fill="rgba(255,255,255,.18)"/>
    <text x="1064" y="596" text-anchor="middle" class="layer-sub">fork + Seccomp + childTryOpen()</text>
  </g>
  <line x1="360" y1="600" x2="360" y2="640" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <line x1="940" y1="600" x2="940" y2="640" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <g filter="url(#shadow)">
    <rect x="80" y="640" width="560" height="130" rx="12" fill="url(#osGrad)"/>
    <text x="104" y="670" class="layer-title">Android Framework / {"\u6587\u4ef6\u7cfb\u7edf"}</text>
    <text x="104" y="690" class="layer-sub">{"\u6bcf\u4e2a APK \u72ec\u7acb UID + \u79c1\u6709\u76ee\u5f55 ACL"}</text>
    <rect x="104" y="702" width="240" height="52" rx="8" fill="rgba(255,255,255,.12)"/>
    <text x="120" y="724" class="layer-sub">/data/data/&lt;package&gt;/files/</text>
    <text x="120" y="742" class="layer-sub">{"\u8de8 UID \u8bbf\u95ee = Permission Denied"}</text>
    <rect x="364" y="702" width="250" height="52" rx="8" fill="rgba(255,255,255,.12)"/>
    <text x="380" y="724" class="layer-sub">Process.myUid() {"\u6807\u8bc6\u5e94\u7528\u8eab\u4efd"}</text>
    <text x="380" y="742" class="layer-sub">File.canRead() {"\u89e6\u53d1\u5185\u6838\u6743\u9650\u68c0\u67e5"}</text>
  </g>
  <g filter="url(#shadow)">
    <rect x="680" y="640" width="520" height="130" rx="12" fill="url(#osGrad)"/>
    <text x="704" y="670" class="layer-title">Linux Kernel + Seccomp BPF</text>
    <text x="704" y="690" class="layer-sub">prctl + seccomp(SECCOMP_MODE_FILTER)</text>
    <rect x="704" y="702" width="240" height="52" rx="8" fill="rgba(255,255,255,.12)"/>
    <text x="720" y="724" class="layer-sub">{"\u5141\u8bb8: \u9ed8\u8ba4 ALLOW"}</text>
    <text x="720" y="742" class="layer-sub">{"\u62e6\u622a: SYS_openat = EPERM"}</text>
    <rect x="964" y="702" width="220" height="52" rx="8" fill="rgba(255,255,255,.12)"/>
    <text x="980" y="724" class="layer-sub">fork() {"\u5b50\u8fdb\u7a0b\u9694\u79bb"}</text>
    <text x="980" y="742" class="layer-sub">waitpid() {"\u6536\u96c6\u9000\u51fa\u7801"}</text>
  </g>
  <rect x="80" y="792" width="1120" height="108" rx="12" fill="#fff" stroke="#cfd8dc" filter="url(#shadow)"/>
  <text x="104" y="818" class="box-title">{"\u6570\u636e\u6d41\u4e0e\u4f9d\u8d56\u5173\u7cfb"}</text>
  <text x="104" y="842" class="box-text">{"\u7528\u6237\u70b9\u51fb -\u003e MainActivity -\u003e SandboxDemo/SandboxNative -\u003e appendLog -\u003e log_text"}</text>
  <text x="104" y="862" class="box-text">Demo {"\u2462\u2463"}: prepareNativeTestFile() {"\u751f\u6210 cacheDir/native_test.txt \u518d\u7ecf JNI \u4f20\u5165 Native"}</text>
  <text x="104" y="882" class="box-text">{"\u6784\u5efa\u94fe: build.gradle.kts -\u003e CMakeLists.txt -\u003e libsan.so -\u003e System.loadLibrary"}</text>
  <rect x="980" y="88" width="220" height="132" rx="10" fill="#fff" stroke="#cfd8dc"/>
  <text x="1000" y="112" class="box-title">{"\u56fe\u4f8b"}</text>
  <rect x="1000" y="124" width="16" height="16" rx="3" fill="url(#uiGrad)"/>
  <text x="1024" y="137" class="legend-text">UI {"\u8868\u73b0\u5c42"}</text>
  <rect x="1000" y="148" width="16" height="16" rx="3" fill="url(#ctrlGrad)"/>
  <text x="1024" y="161" class="legend-text">Activity {"\u63a7\u5236"}</text>
  <rect x="1000" y="172" width="16" height="16" rx="3" fill="url(#bizGrad)"/>
  <text x="1024" y="185" class="legend-text">JVM {"\u6c99\u7bb1\u903b\u8f91"}</text>
  <rect x="1000" y="196" width="16" height="16" rx="3" fill="url(#jniGrad)"/>
  <text x="1024" y="209" class="legend-text">JNI {"\u6865\u63a5"}</text>
</svg>'''


def flow_svg() -> str:
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1400 2480" width="1400" height="2480">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#fafbff"/><stop offset="100%" stop-color="#eef2fb"/></linearGradient>
    <linearGradient id="startGrad" x1="0" y1="0" x2="1" y2="1"><stop offset="0%" stop-color="#7e57c2"/><stop offset="100%" stop-color="#5e35b1"/></linearGradient>
    <linearGradient id="procGrad" x1="0" y1="0" x2="1" y2="0"><stop offset="0%" stop-color="#fff"/><stop offset="100%" stop-color="#f5f7ff"/></linearGradient>
    <linearGradient id="okGrad" x1="0" y1="0" x2="1" y2="0"><stop offset="0%" stop-color="#66bb6a"/><stop offset="100%" stop-color="#43a047"/></linearGradient>
    <linearGradient id="failGrad" x1="0" y1="0" x2="1" y2="0"><stop offset="0%" stop-color="#ef5350"/><stop offset="100%" stop-color="#e53935"/></linearGradient>
    <linearGradient id="warnGrad" x1="0" y1="0" x2="1" y2="0"><stop offset="0%" stop-color="#ffa726"/><stop offset="100%" stop-color="#fb8c00"/></linearGradient>
    <linearGradient id="lane1" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#e8f5e9"/><stop offset="100%" stop-color="#f1f8e9"/></linearGradient>
    <linearGradient id="lane2" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#ffebee"/><stop offset="100%" stop-color="#fce4ec"/></linearGradient>
    <linearGradient id="lane3" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#e3f2fd"/><stop offset="100%" stop-color="#e8eaf6"/></linearGradient>
    <linearGradient id="lane4" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#fff3e0"/><stop offset="100%" stop-color="#fbe9e7"/></linearGradient>
    <marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="#546e7a"/></marker>
    <marker id="arrowGreen" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="#2e7d32"/></marker>
    <marker id="arrowRed" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="#c62828"/></marker>
    <filter id="shadow" x="-15%" y="-15%" width="130%" height="130%"><feDropShadow dx="0" dy="2" stdDeviation="3" flood-color="#000" flood-opacity=".1"/></filter>
    <style>
      .title {{ font:700 28px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#1a237e; }}
      .subtitle {{ font:400 14px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#546e7a; }}
      .section {{ font:700 18px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#283593; }}
      .lane-title {{ font:700 15px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#37474f; }}
      .node-title {{ font:700 13px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#263238; }}
      .node-text {{ font:400 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#455a64; }}
      .mono {{ font:400 11px Consolas,monospace; fill:#37474f; }}
      .decision {{ font:700 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#e65100; }}
      .edge-label {{ font:400 11px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#607d8b; }}
      .result-ok {{ font:700 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#fff; }}
      .result-fail {{ font:700 12px "Segoe UI","PingFang SC","Microsoft YaHei",sans-serif; fill:#fff; }}
    </style>
  </defs>
  <rect width="1400" height="2480" fill="url(#bg)"/>
  <text x="700" y="42" text-anchor="middle" class="title">{"\u6c99\u7bb1 Demo \u8be6\u7ec6\u6d41\u7a0b\u56fe"}</text>
  <text x="700" y="68" text-anchor="middle" class="subtitle">{"\u542f\u52a8\u6d41\u7a0b + \u56db\u4e2a\u6f14\u793a\u573a\u666f\u7684\u5b8c\u6574\u6267\u884c\u8def\u5f84"}</text>

  <text x="60" y="108" class="section">A. {"\u5e94\u7528\u542f\u52a8\u6d41\u7a0b"}</text>
  <rect x="40" y="120" width="1320" height="250" rx="14" fill="#fff" stroke="#c5cae9" filter="url(#shadow)"/>
  <ellipse cx="700" cy="160" rx="88" ry="28" fill="url(#startGrad)"/>
  <text x="700" y="165" text-anchor="middle" class="result-ok">{"\u542f\u52a8 App"}</text>
  <line x1="700" y1="188" x2="700" y2="208" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="520" y="210" width="360" height="44" rx="10" fill="url(#procGrad)" stroke="#9fa8da"/>
  <text x="700" y="228" text-anchor="middle" class="node-title">MainActivity.onCreate()</text>
  <text x="700" y="244" text-anchor="middle" class="node-text">ActivityMainBinding.inflate + setContentView</text>
  <line x1="700" y1="254" x2="700" y2="274" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="470" y="276" width="460" height="44" rx="10" fill="url(#procGrad)" stroke="#9fa8da"/>
  <text x="700" y="294" text-anchor="middle" class="node-title">SandboxDemo.appInfo(context)</text>
  <text x="700" y="310" text-anchor="middle" class="mono">{"\u663e\u793a UID \u548c filesDir \u5230 info_text"}</text>
  <line x1="700" y1="320" x2="700" y2="340" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="430" y="342" width="540" height="44" rx="10" fill="url(#procGrad)" stroke="#9fa8da"/>
  <text x="700" y="360" text-anchor="middle" class="node-title">{"\u6ce8\u518c 4 \u4e2a\u6309\u94ae\u76d1\u542c + appendLog(\u5c31\u7eea)"}</text>
  <text x="700" y="376" text-anchor="middle" class="node-text">{"\u7b49\u5f85\u7528\u6237\u70b9\u51fb"}</text>

  <text x="60" y="410" class="section">B. {"\u901a\u7528\u6f14\u793a\u6267\u884c\u6d41\u7a0b"}</text>
  <rect x="40" y="422" width="1320" height="170" rx="14" fill="#fff" stroke="#c5cae9" filter="url(#shadow)"/>
  <rect x="590" y="442" width="220" height="36" rx="18" fill="url(#warnGrad)"/>
  <text x="700" y="465" text-anchor="middle" class="result-ok">{"\u7528\u6237\u70b9\u51fb\u6309\u94ae"}</text>
  <line x1="700" y1="478" x2="700" y2="498" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="520" y="500" width="360" height="36" rx="10" fill="url(#procGrad)" stroke="#9fa8da"/>
  <text x="700" y="523" text-anchor="middle" class="node-title">MainActivity.runDemo(title) {{ block() }}</text>
  <line x1="700" y1="536" x2="700" y2="556" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="500" y="558" width="400" height="36" rx="10" fill="url(#procGrad)" stroke="#9fa8da"/>
  <text x="700" y="581" text-anchor="middle" class="node-title">appendLog("[HH:mm:ss] title + result")</text>

  <text x="60" y="630" class="section">C. Demo {"\u2460"} {"\u8bfb\u5199\u5e94\u7528\u79c1\u6709\u76ee\u5f55 (\u5141\u8bb8)"}</text>
  <rect x="40" y="642" width="1320" height="360" rx="14" fill="url(#lane1)" stroke="#a5d6a7" filter="url(#shadow)"/>
  <text x="60" y="668" class="lane-title">SandboxDemo.demoPrivateFileAccess()</text>
  <rect x="560" y="684" width="280" height="34" rx="10" fill="url(#procGrad)" stroke="#81c784"/>
  <text x="700" y="706" text-anchor="middle" class="node-title">File(filesDir, "sandbox_secret.txt")</text>
  <line x1="700" y1="718" x2="700" y2="738" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <rect x="560" y="740" width="280" height="34" rx="10" fill="url(#procGrad)" stroke="#81c784"/>
  <text x="700" y="762" text-anchor="middle" class="node-title">file.writeText("token=timestamp")</text>
  <line x1="700" y1="774" x2="700" y2="794" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <rect x="560" y="796" width="280" height="34" rx="10" fill="url(#procGrad)" stroke="#81c784"/>
  <text x="700" y="818" text-anchor="middle" class="node-title">readBack = file.readText()</text>
  <line x1="700" y1="830" x2="700" y2="850" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <polygon points="700,850 780,890 700,930 620,890" fill="#fff8e1" stroke="#fb8c00" stroke-width="2"/>
  <text x="700" y="894" text-anchor="middle" class="decision">readBack == secret ?</text>
  <line x1="780" y1="890" x2="930" y2="890" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <text x="840" y="882" class="edge-label">{"\u662f"}</text>
  <rect x="930" y="872" width="260" height="36" rx="10" fill="url(#okGrad)"/>
  <text x="1060" y="895" text-anchor="middle" class="result-ok">[OK] {"\u5199\u5165\u5e76\u8bfb\u56de\u79c1\u6709\u6587\u4ef6"}</text>
  <line x1="620" y1="890" x2="470" y2="890" stroke="#c62828" stroke-width="2" marker-end="url(#arrowRed)"/>
  <text x="545" y="882" class="edge-label">{"\u5426"}</text>
  <rect x="210" y="872" width="260" height="36" rx="10" fill="url(#failGrad)"/>
  <text x="340" y="895" text-anchor="middle" class="result-fail">[FAIL] {"\u5f02\u5e38\u6216\u5931\u8d25"}</text>
  <rect x="80" y="948" width="420" height="42" rx="8" fill="#fff" stroke="#a5d6a7"/>
  <text x="96" y="968" class="node-text">{"\u540c UID \u8bbf\u95ee /data/data/com.example.san/files/"}</text>
  <text x="96" y="984" class="mono">VFS {"\u6743\u9650\u68c0\u67e5\u901a\u8fc7"}</text>

  <text x="60" y="1040" class="section">D. Demo {"\u2461"} {"\u8d8a\u754c\u8bbf\u95ee\u5176\u4ed6\u5e94\u7528\u76ee\u5f55 (\u62d2\u7edd)"}</text>
  <rect x="40" y="1052" width="1320" height="340" rx="14" fill="url(#lane2)" stroke="#ef9a9a" filter="url(#shadow)"/>
  <text x="60" y="1078" class="lane-title">SandboxDemo.demoCrossBoundaryRead()</text>
  <rect x="500" y="1094" width="400" height="34" rx="10" fill="url(#procGrad)" stroke="#e57373"/>
  <text x="700" y="1116" text-anchor="middle" class="node-title">File("/data/data/com.android.settings")</text>
  <line x1="700" y1="1128" x2="700" y2="1148" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="540" y="1150" width="320" height="34" rx="10" fill="url(#procGrad)" stroke="#e57373"/>
  <text x="700" y="1172" text-anchor="middle" class="node-title">foreignFile.canRead()</text>
  <line x1="700" y1="1184" x2="700" y2="1204" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <polygon points="700,1204 780,1244 700,1284 620,1244" fill="#fff8e1" stroke="#fb8c00" stroke-width="2"/>
  <text x="700" y="1248" text-anchor="middle" class="decision">{"\u53ef\u8bfb?"}</text>
  <line x1="620" y1="1244" x2="470" y2="1244" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <text x="545" y="1236" class="edge-label">{"\u5426 (\u6b63\u5e38)"}</text>
  <rect x="210" y="1226" width="260" height="36" rx="10" fill="url(#okGrad)"/>
  <text x="340" y="1249" text-anchor="middle" class="result-ok">[OK] {"\u4e0d\u53ef\u8bfb"}</text>
  <line x1="780" y1="1244" x2="930" y2="1244" stroke="#c62828" stroke-width="2" marker-end="url(#arrowRed)"/>
  <text x="840" y="1236" class="edge-label">{"\u662f (\u5f02\u5e38)"}</text>
  <rect x="930" y="1226" width="260" height="36" rx="10" fill="url(#warnGrad)"/>
  <text x="1060" y="1249" text-anchor="middle" class="result-ok">[WARN] {"\u53ef\u8bfb"}</text>
  <line x1="700" y1="1284" x2="700" y2="1304" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="500" y="1306" width="400" height="34" rx="10" fill="url(#procGrad)" stroke="#e57373"/>
  <text x="700" y="1328" text-anchor="middle" class="node-title">{"\u5bf9\u6bd4 ownDir.canRead() = \u672c\u5e94\u7528\u53ef\u8bfb"}</text>
  <line x1="700" y1="1340" x2="700" y2="1360" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="430" y="1362" width="540" height="36" rx="10" fill="url(#failGrad)"/>
  <text x="700" y="1385" text-anchor="middle" class="result-fail">{"\u7ed3\u8bba: Android \u6309 UID \u9694\u79bb /data/data/"}</text>

  <text x="60" y="1430" class="section">E. Demo {"\u2462"} Native {"\u65e0 Seccomp (\u5141\u8bb8)"}</text>
  <rect x="40" y="1442" width="1320" height="380" rx="14" fill="url(#lane3)" stroke="#90caf9" filter="url(#shadow)"/>
  <text x="60" y="1468" class="lane-title">prepareNativeTestFile() + runNativeFileRead(path)</text>
  <rect x="520" y="1484" width="360" height="34" rx="10" fill="url(#procGrad)" stroke="#64b5f6"/>
  <text x="700" y="1506" text-anchor="middle" class="node-title">cacheDir/native_test.txt {"\u5199\u5165 payload"}</text>
  <line x1="700" y1="1518" x2="700" y2="1538" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="520" y="1540" width="360" height="34" rx="10" fill="url(#procGrad)" stroke="#64b5f6"/>
  <text x="700" y="1562" text-anchor="middle" class="node-title">JNI runNativeFileRead(path)</text>
  <line x1="700" y1="1574" x2="700" y2="1594" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="500" y="1596" width="400" height="48" rx="10" fill="#fff3e0" stroke="#ffb74d"/>
  <text x="700" y="1616" text-anchor="middle" class="node-title">readFilePreview(path)</text>
  <text x="700" y="1634" text-anchor="middle" class="mono">openat + read + close</text>
  <line x1="700" y1="1644" x2="700" y2="1664" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <polygon points="700,1664 780,1704 700,1744 620,1704" fill="#fff8e1" stroke="#fb8c00" stroke-width="2"/>
  <text x="700" y="1708" text-anchor="middle" class="decision">openat {"\u6210\u529f?"}</text>
  <line x1="780" y1="1704" x2="930" y2="1704" stroke="#2e7d32" stroke-width="2" marker-end="url(#arrowGreen)"/>
  <text x="840" y="1696" class="edge-label">{"\u662f"}</text>
  <rect x="930" y="1686" width="300" height="36" rx="10" fill="url(#okGrad)"/>
  <text x="1080" y="1709" text-anchor="middle" class="result-ok">[OK] {"\u8bfb\u53d6 N \u5b57\u8282 + \u5185\u5bb9\u9884\u89c8"}</text>
  <line x1="620" y1="1704" x2="470" y2="1704" stroke="#c62828" stroke-width="2" marker-end="url(#arrowRed)"/>
  <text x="545" y="1696" class="edge-label">{"\u5426"}</text>
  <rect x="210" y="1686" width="260" height="36" rx="10" fill="url(#failGrad)"/>
  <text x="340" y="1709" text-anchor="middle" class="result-fail">[FAIL] open/read + errno</text>

  <text x="60" y="1860" class="section">F. Demo {"\u2463"} Seccomp {"\u62e6\u622a openat (\u62d2\u7edd)"}</text>
  <rect x="40" y="1872" width="1320" height="560" rx="14" fill="url(#lane4)" stroke="#ffcc80" filter="url(#shadow)"/>
  <text x="60" y="1898" class="lane-title">runSeccompBlockedRead(path) + fork {"\u5b50\u8fdb\u7a0b\u9694\u79bb"}</text>
  <rect x="560" y="1914" width="280" height="34" rx="10" fill="url(#procGrad)" stroke="#ffb74d"/>
  <text x="700" y="1936" text-anchor="middle" class="node-title">prepareNativeTestFile()</text>
  <line x1="700" y1="1948" x2="700" y2="1968" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="560" y="1970" width="280" height="34" rx="10" fill="url(#procGrad)" stroke="#ffb74d"/>
  <text x="700" y="1992" text-anchor="middle" class="node-title">pid = fork()</text>
  <line x1="700" y1="2004" x2="700" y2="2024" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <polygon points="700,2024 780,2064 700,2104 620,2064" fill="#fff8e1" stroke="#fb8c00" stroke-width="2"/>
  <text x="700" y="2068" text-anchor="middle" class="decision">pid == 0 ?</text>
  <line x1="620" y1="2064" x2="380" y2="2064" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <text x="490" y="2056" class="edge-label">{"\u5b50\u8fdb\u7a0b"}</text>
  <rect x="80" y="2084" width="300" height="40" rx="10" fill="#fff3e0" stroke="#ffb74d"/>
  <text x="230" y="2102" text-anchor="middle" class="node-title">installOpenBlockFilter()</text>
  <text x="230" y="2118" text-anchor="middle" class="mono">prctl + seccomp BPF</text>
  <line x1="230" y1="2124" x2="230" y2="2144" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="80" y="2146" width="300" height="40" rx="10" fill="#fff3e0" stroke="#ffb74d"/>
  <text x="230" y="2164" text-anchor="middle" class="node-title">openat(path) {"\u88ab\u62e6\u622a"}</text>
  <text x="230" y="2180" text-anchor="middle" class="mono">SYS_openat = EPERM</text>
  <line x1="230" y1="2186" x2="230" y2="2206" stroke="#c62828" stroke-width="2" marker-end="url(#arrowRed)"/>
  <rect x="130" y="2208" width="200" height="32" rx="8" fill="url(#failGrad)"/>
  <text x="230" y="2229" text-anchor="middle" class="result-fail">_exit(1)</text>
  <line x1="780" y1="2064" x2="1020" y2="2064" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <text x="880" y="2056" class="edge-label">{"\u7236\u8fdb\u7a0b"}</text>
  <rect x="900" y="2044" width="280" height="40" rx="10" fill="url(#procGrad)" stroke="#ffb74d"/>
  <text x="1040" y="2062" text-anchor="middle" class="node-title">waitpid(pid, status)</text>
  <text x="1040" y="2078" text-anchor="middle" class="node-text">{"\u7b49\u5f85\u5b50\u8fdb\u7a0b\u7ed3\u675f"}</text>
  <line x1="1040" y1="2084" x2="1040" y2="2104" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="880" y="2106" width="320" height="48" rx="10" fill="url(#procGrad)" stroke="#ffb74d"/>
  <text x="1040" y="2126" text-anchor="middle" class="node-title">decodeChildStatus(status)</text>
  <text x="1040" y="2144" text-anchor="middle" class="mono">WIFEXITED / WIFSIGNALED</text>
  <line x1="1040" y1="2154" x2="1040" y2="2174" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="860" y="2176" width="360" height="56" rx="10" fill="url(#failGrad)"/>
  <text x="1040" y="2196" text-anchor="middle" class="result-fail">[BLOCK] openat() EPERM</text>
  <text x="1040" y="2214" text-anchor="middle" class="result-fail">{"\u6216 SIGSYS \u88ab\u5185\u6838\u7ec8\u6b62"}</text>
  <line x1="1040" y1="2232" x2="1040" y2="2252" stroke="#546e7a" stroke-width="2" marker-end="url(#arrow)"/>
  <rect x="860" y="2254" width="360" height="34" rx="10" fill="url(#procGrad)" stroke="#ffb74d"/>
  <text x="1040" y="2276" text-anchor="middle" class="node-title">{"\u8fd4\u56de Java String -\u003e appendLog"}</text>

  <rect x="40" y="2380" width="1320" height="72" rx="12" fill="#fff" stroke="#c5cae9" filter="url(#shadow)"/>
  <text x="60" y="2406" class="node-title">{"\u6d41\u7a0b\u603b\u7ed3"}</text>
  <text x="60" y="2428" class="node-text">Demo {"\u2460\u2463"} {"\u6b63\u5e38\u884c\u4e3a"}; Demo {"\u2461"} UID {"\u9694\u79bb"}; Demo {"\u2463"} Seccomp {"\u5b50\u8fdb\u7a0b\u62e6\u622a\u4e0d\u5f71\u54cd\u4e3b\u8fdb\u7a0b"}</text>
  <text x="60" y="2448" class="mono">File.writeText | canRead | JNI | fork | prctl | seccomp | openat | waitpid</text>
</svg>'''


def main() -> None:
    (DOCS / "sandbox-demo-architecture.svg").write_text(arch_svg(), encoding="utf-8")
    (DOCS / "sandbox-demo-flow.svg").write_text(flow_svg(), encoding="utf-8")
    print("Generated:")
    print(" - sandbox-demo-architecture.svg")
    print(" - sandbox-demo-flow.svg")


if __name__ == "__main__":
    main()
