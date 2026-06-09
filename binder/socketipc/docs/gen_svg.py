# -*- coding: utf-8 -*-
import pathlib

ARCH = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 820" font-family="Segoe UI, Microsoft YaHei, sans-serif">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#f8fafc"/>
      <stop offset="100%" stop-color="#eef2f7"/>
    </linearGradient>
    <linearGradient id="mainProc" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#dbeafe"/>
      <stop offset="100%" stop-color="#bfdbfe"/>
    </linearGradient>
    <linearGradient id="binderProc" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#dcfce7"/>
      <stop offset="100%" stop-color="#bbf7d0"/>
    </linearGradient>
    <linearGradient id="socketProc" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#ffedd5"/>
      <stop offset="100%" stop-color="#fed7aa"/>
    </linearGradient>
    <linearGradient id="nativeLib" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#ede9fe"/>
      <stop offset="100%" stop-color="#ddd6fe"/>
    </linearGradient>
    <marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#475569"/>
    </marker>
    <marker id="arrowGreen" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#15803d"/>
    </marker>
    <marker id="arrowOrange" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#c2410c"/>
    </marker>
    <filter id="shadow" x="-4%" y="-4%" width="108%" height="108%">
      <feDropShadow dx="0" dy="2" stdDeviation="3" flood-color="#000" flood-opacity="0.12"/>
    </filter>
  </defs>
  <rect width="1200" height="820" fill="url(#bg)"/>
  <text x="600" y="42" text-anchor="middle" font-size="26" font-weight="700" fill="#0f172a">SocketIPC \u9879\u76ee\u67b6\u6784\u56fe</text>
  <text x="600" y="68" text-anchor="middle" font-size="14" fill="#64748b">Binder IPC vs Unix Domain Socket IPC / C++20 Native / \u4e09\u8fdb\u7a0b\u6a21\u578b</text>
  <rect x="40" y="88" width="1120" height="56" rx="10" fill="#1e293b" filter="url(#shadow)"/>
  <text x="600" y="122" text-anchor="middle" font-size="15" font-weight="600" fill="#f8fafc">Android Framework / Linux Kernel</text>
  <rect x="80" y="168" width="220" height="48" rx="8" fill="#334155" filter="url(#shadow)"/>
  <text x="190" y="198" text-anchor="middle" font-size="13" font-weight="600" fill="#e2e8f0">Binder Driver (/dev/binder)</text>
  <rect x="900" y="168" width="220" height="48" rx="8" fill="#334155" filter="url(#shadow)"/>
  <text x="1010" y="192" text-anchor="middle" font-size="13" font-weight="600" fill="#e2e8f0">Unix Domain Socket</text>
  <text x="1010" y="208" text-anchor="middle" font-size="11" fill="#94a3b8">abstract:socketipc_calc</text>
  <rect x="40" y="240" width="360" height="340" rx="14" fill="url(#mainProc)" stroke="#3b82f6" stroke-width="2" filter="url(#shadow)"/>
  <rect x="40" y="240" width="360" height="36" rx="14" fill="#2563eb"/>
  <rect x="40" y="262" width="360" height="14" fill="#2563eb"/>
  <text x="220" y="264" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">\u4e3b\u8fdb\u7a0b (Main Process)</text>
  <text x="220" y="282" text-anchor="middle" font-size="11" fill="#1e40af">com.example.myapplication</text>
  <rect x="60" y="300" width="320" height="52" rx="8" fill="#fff" stroke="#93c5fd"/>
  <text x="220" y="322" text-anchor="middle" font-size="13" font-weight="600" fill="#1e3a8a">MainActivity (Kotlin UI)</text>
  <text x="220" y="340" text-anchor="middle" font-size="11" fill="#64748b">bindService / \u538b\u6d4b\u5165\u53e3 / \u65e5\u5fd7\u5c55\u793a</text>
  <rect x="60" y="362" width="150" height="44" rx="8" fill="#fff" stroke="#93c5fd"/>
  <text x="135" y="382" text-anchor="middle" font-size="12" font-weight="600" fill="#1e3a8a">ICalculator</text>
  <text x="135" y="398" text-anchor="middle" font-size="10" fill="#64748b">AIDL Client Stub</text>
  <rect x="230" y="362" width="150" height="44" rx="8" fill="#fff" stroke="#93c5fd"/>
  <text x="305" y="382" text-anchor="middle" font-size="12" font-weight="600" fill="#1e3a8a">JNI Bridge</text>
  <text x="305" y="398" text-anchor="middle" font-size="10" fill="#64748b">nativeWait / nativeAdd</text>
  <rect x="60" y="418" width="320" height="44" rx="8" fill="#fff" stroke="#93c5fd"/>
  <text x="220" y="438" text-anchor="middle" font-size="12" font-weight="600" fill="#1e3a8a">BinderBenchmark / NativeIpc</text>
  <text x="220" y="454" text-anchor="middle" font-size="10" fill="#64748b">\u591a\u7ebf\u7a0b\u538b\u6d4b / CustomCodec / ParcelCodec</text>
  <rect x="60" y="474" width="320" height="44" rx="8" fill="#fff" stroke="#93c5fd"/>
  <text x="220" y="494" text-anchor="middle" font-size="12" font-weight="600" fill="#1e3a8a">SocketClient (C++)</text>
  <text x="220" y="510" text-anchor="middle" font-size="10" fill="#64748b">ping / add / process_custom / parcel</text>
  <rect x="60" y="530" width="320" height="36" rx="8" fill="#eff6ff" stroke="#60a5fa" stroke-dasharray="4,3"/>
  <text x="220" y="553" text-anchor="middle" font-size="11" fill="#1d4ed8">libmyapplication.so (Client \u90e8\u5206)</text>
  <rect x="420" y="240" width="360" height="340" rx="14" fill="url(#binderProc)" stroke="#22c55e" stroke-width="2" filter="url(#shadow)"/>
  <rect x="420" y="240" width="360" height="36" rx="14" fill="#16a34a"/>
  <rect x="420" y="262" width="360" height="14" fill="#16a34a"/>
  <text x="600" y="264" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">Binder \u8fdc\u7a0b\u8fdb\u7a0b</text>
  <text x="600" y="282" text-anchor="middle" font-size="11" fill="#14532d">:binder_remote</text>
  <rect x="440" y="300" width="320" height="52" rx="8" fill="#fff" stroke="#86efac"/>
  <text x="600" y="322" text-anchor="middle" font-size="13" font-weight="600" fill="#14532d">CalculatorService (Kotlin)</text>
  <text x="600" y="340" text-anchor="middle" font-size="11" fill="#64748b">ICalculator.Stub / onBind</text>
  <rect x="440" y="362" width="320" height="44" rx="8" fill="#fff" stroke="#86efac"/>
  <text x="600" y="382" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">AIDL \u63a5\u53e3\u5c42</text>
  <text x="600" y="398" text-anchor="middle" font-size="10" fill="#64748b">add / processCustom / processParcel</text>
  <rect x="440" y="418" width="320" height="44" rx="8" fill="#fff" stroke="#86efac"/>
  <text x="600" y="438" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">NativeIpc (JNI Server)</text>
  <text x="600" y="454" text-anchor="middle" font-size="10" fill="#64748b">nativeProcessCustom / ParcelPayload</text>
  <rect x="440" y="474" width="320" height="44" rx="8" fill="#fff" stroke="#86efac"/>
  <text x="600" y="494" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">\u4e1a\u52a1\u903b\u8f91</text>
  <text x="600" y="510" text-anchor="middle" font-size="10" fill="#64748b">\u6574\u6570\u52a0\u6cd5 / UserProfile \u7f16\u89e3\u7801</text>
  <rect x="440" y="530" width="320" height="36" rx="8" fill="#f0fdf4" stroke="#4ade80" stroke-dasharray="4,3"/>
  <text x="600" y="553" text-anchor="middle" font-size="11" fill="#15803d">libmyapplication.so (Server \u7f16\u89e3\u7801)</text>
  <rect x="800" y="240" width="360" height="340" rx="14" fill="url(#socketProc)" stroke="#f97316" stroke-width="2" filter="url(#shadow)"/>
  <rect x="800" y="240" width="360" height="36" rx="14" fill="#ea580c"/>
  <rect x="800" y="262" width="360" height="14" fill="#ea580c"/>
  <text x="980" y="264" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">Socket \u8fdc\u7a0b\u8fdb\u7a0b</text>
  <text x="980" y="282" text-anchor="middle" font-size="11" fill="#7c2d12">:socket_remote</text>
  <rect x="820" y="300" width="320" height="52" rx="8" fill="#fff" stroke="#fdba74"/>
  <text x="980" y="322" text-anchor="middle" font-size="13" font-weight="600" fill="#9a3412">SocketServerService (Kotlin)</text>
  <text x="980" y="340" text-anchor="middle" font-size="11" fill="#64748b">onCreate / nativeStartSocketServer</text>
  <rect x="820" y="362" width="320" height="44" rx="8" fill="#fff" stroke="#fdba74"/>
  <text x="980" y="382" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">SocketServer (C++)</text>
  <text x="980" y="398" text-anchor="middle" font-size="10" fill="#64748b">listen / accept / \u591a\u7ebf\u7a0b\u5904\u7406</text>
  <rect x="820" y="418" width="320" height="44" rx="8" fill="#fff" stroke="#fdba74"/>
  <text x="980" y="438" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">SocketProtocol \u5e27\u534f\u8bae</text>
  <text x="980" y="454" text-anchor="middle" font-size="10" fill="#64748b">[opcode][length][payload]</text>
  <rect x="820" y="474" width="320" height="44" rx="8" fill="#fff" stroke="#fdba74"/>
  <text x="980" y="494" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">Opcode \u5206\u53d1</text>
  <text x="980" y="510" text-anchor="middle" font-size="10" fill="#64748b">kAdd / kProcessCustom / kProcessParcel / kPing</text>
  <rect x="820" y="530" width="320" height="36" rx="8" fill="#fff7ed" stroke="#fb923c" stroke-dasharray="4,3"/>
  <text x="980" y="553" text-anchor="middle" font-size="11" fill="#c2410c">libmyapplication.so (Server \u5168\u91cf)</text>
  <rect x="120" y="610" width="960" height="120" rx="14" fill="url(#nativeLib)" stroke="#8b5cf6" stroke-width="2" filter="url(#shadow)"/>
  <text x="600" y="638" text-anchor="middle" font-size="15" font-weight="700" fill="#5b21b6">\u5171\u4eab Native \u6a21\u5757 (C++20) - libmyapplication.so</text>
  <rect x="140" y="654" width="170" height="58" rx="8" fill="#fff" stroke="#c4b5fd"/>
  <text x="225" y="678" text-anchor="middle" font-size="12" font-weight="600" fill="#4c1d95">types.hpp</text>
  <text x="225" y="696" text-anchor="middle" font-size="10" fill="#64748b">UserProfile</text>
  <rect x="330" y="654" width="170" height="58" rx="8" fill="#fff" stroke="#c4b5fd"/>
  <text x="415" y="678" text-anchor="middle" font-size="12" font-weight="600" fill="#4c1d95">CustomCodec</text>
  <text x="415" y="696" text-anchor="middle" font-size="10" fill="#64748b">IPCC \u7d27\u51d1\u4e8c\u8fdb\u5236</text>
  <rect x="520" y="654" width="170" height="58" rx="8" fill="#fff" stroke="#c4b5fd"/>
  <text x="605" y="678" text-anchor="middle" font-size="12" font-weight="600" fill="#4c1d95">ParcelCodec</text>
  <text x="605" y="696" text-anchor="middle" font-size="10" fill="#64748b">Parcel 4 \u5b57\u8282\u5bf9\u9f50</text>
  <rect x="710" y="654" width="170" height="58" rx="8" fill="#fff" stroke="#c4b5fd"/>
  <text x="795" y="678" text-anchor="middle" font-size="12" font-weight="600" fill="#4c1d95">socket_protocol</text>
  <text x="795" y="696" text-anchor="middle" font-size="10" fill="#64748b">\u5e27\u7f16\u89e3\u7801</text>
  <rect x="900" y="654" width="160" height="58" rx="8" fill="#fff" stroke="#c4b5fd"/>
  <text x="980" y="678" text-anchor="middle" font-size="12" font-weight="600" fill="#4c1d95">benchmark</text>
  <text x="980" y="696" text-anchor="middle" font-size="10" fill="#64748b">\u591a\u7ebf\u7a0b\u538b\u6d4b</text>
  <path d="M 220 240 L 220 216 L 190 216" fill="none" stroke="#15803d" stroke-width="2.5" marker-end="url(#arrowGreen)"/>
  <path d="M 190 216 L 420 216 L 600 240" fill="none" stroke="#15803d" stroke-width="2.5" marker-end="url(#arrowGreen)"/>
  <text x="310" y="208" font-size="11" font-weight="600" fill="#15803d">Binder IPC (AIDL + Parcel)</text>
  <path d="M 380 400 L 880 216 L 1010 216" fill="none" stroke="#c2410c" stroke-width="2.5" marker-end="url(#arrowOrange)"/>
  <text x="680" y="200" font-size="11" font-weight="600" fill="#c2410c">Socket IPC (Unix Domain)</text>
  <line x1="380" y1="326" x2="440" y2="326" stroke="#475569" stroke-width="1.5" stroke-dasharray="6,4" marker-end="url(#arrow)"/>
  <text x="410" y="316" font-size="10" fill="#475569">bindService</text>
  <line x1="380" y1="340" x2="820" y2="326" stroke="#475569" stroke-width="1.5" stroke-dasharray="6,4" marker-end="url(#arrow)"/>
  <text x="590" y="316" font-size="10" fill="#475569">bindService</text>
  <rect x="40" y="752" width="1120" height="52" rx="10" fill="#fff" stroke="#cbd5e1" filter="url(#shadow)"/>
  <rect x="60" y="770" width="16" height="16" rx="3" fill="#2563eb"/>
  <text x="84" y="783" font-size="12" fill="#334155">\u4e3b\u8fdb\u7a0b UI + Client</text>
  <rect x="230" y="770" width="16" height="16" rx="3" fill="#16a34a"/>
  <text x="254" y="783" font-size="12" fill="#334155">Binder \u670d\u52a1\u7aef</text>
  <rect x="390" y="770" width="16" height="16" rx="3" fill="#ea580c"/>
  <text x="414" y="783" font-size="12" fill="#334155">Socket \u670d\u52a1\u7aef</text>
  <rect x="560" y="770" width="16" height="16" rx="3" fill="#7c3aed"/>
  <text x="584" y="783" font-size="12" fill="#334155">\u5171\u4eab C++20 \u5e93</text>
  <line x1="720" y1="778" x2="760" y2="778" stroke="#15803d" stroke-width="2.5" marker-end="url(#arrowGreen)"/>
  <text x="770" y="783" font-size="12" fill="#334155">Binder \u901a\u9053</text>
  <line x1="870" y1="778" x2="910" y2="778" stroke="#c2410c" stroke-width="2.5" marker-end="url(#arrowOrange)"/>
  <text x="920" y="783" font-size="12" fill="#334155">Socket \u901a\u9053</text>
</svg>"""

FLOW = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 1100" font-family="Segoe UI, Microsoft YaHei, sans-serif">
  <defs>
    <linearGradient id="bg2" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#f8fafc"/>
      <stop offset="100%" stop-color="#eef2f7"/>
    </linearGradient>
    <marker id="arr" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#334155"/>
    </marker>
    <marker id="arrGreen" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#16a34a"/>
    </marker>
    <marker id="arrOrange" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto">
      <path d="M0,0 L10,5 L0,10 Z" fill="#ea580c"/>
    </marker>
    <filter id="sh" x="-4%" y="-4%" width="108%" height="108%">
      <feDropShadow dx="0" dy="2" stdDeviation="2" flood-color="#000" flood-opacity="0.1"/>
    </filter>
  </defs>
  <rect width="1200" height="1100" fill="url(#bg2)"/>
  <text x="600" y="42" text-anchor="middle" font-size="26" font-weight="700" fill="#0f172a">SocketIPC \u6d41\u7a0b\u56fe</text>
  <text x="600" y="68" text-anchor="middle" font-size="14" fill="#64748b">\u5e94\u7528\u542f\u52a8 / Binder \u8c03\u7528 / Socket \u8c03\u7528 / \u590d\u6742\u5bf9\u8c61\u4f20\u9012</text>

  <rect x="40" y="90" width="1120" height="230" rx="12" fill="#fff" stroke="#cbd5e1" stroke-width="1.5" filter="url(#sh)"/>
  <rect x="40" y="90" width="1120" height="32" rx="12" fill="#2563eb"/>
  <rect x="40" y="110" width="1120" height="12" fill="#2563eb"/>
  <text x="600" y="112" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">1. \u5e94\u7528\u542f\u52a8\u6d41\u7a0b</text>

  <rect x="70" y="140" width="130" height="44" rx="8" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="135" y="160" text-anchor="middle" font-size="12" font-weight="600" fill="#1e40af">MainActivity</text>
  <text x="135" y="176" text-anchor="middle" font-size="10" fill="#64748b">onCreate</text>
  <line x1="200" y1="162" x2="230" y2="162" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="230" y="140" width="150" height="44" rx="8" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="305" y="158" text-anchor="middle" font-size="12" font-weight="600" fill="#1e40af">bindService</text>
  <text x="305" y="174" text-anchor="middle" font-size="10" fill="#64748b">CalculatorService</text>
  <line x1="380" y1="162" x2="410" y2="162" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="410" y="140" width="170" height="44" rx="8" fill="#dcfce7" stroke="#22c55e"/>
  <text x="495" y="158" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">:binder_remote \u542f\u52a8</text>
  <text x="495" y="174" text-anchor="middle" font-size="10" fill="#64748b">onBind - ICalculator.Stub</text>
  <line x1="580" y1="162" x2="610" y2="162" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="610" y="140" width="150" height="44" rx="8" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="685" y="158" text-anchor="middle" font-size="12" font-weight="600" fill="#1e40af">onServiceConnected</text>
  <text x="685" y="174" text-anchor="middle" font-size="10" fill="#64748b">Binder \u5ba2\u6237\u7aef\u5c31\u7eea</text>

  <rect x="70" y="200" width="150" height="44" rx="8" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="145" y="218" text-anchor="middle" font-size="12" font-weight="600" fill="#1e40af">bindService</text>
  <text x="145" y="234" text-anchor="middle" font-size="10" fill="#64748b">SocketServerService</text>
  <line x1="220" y1="222" x2="250" y2="222" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="250" y="200" width="180" height="44" rx="8" fill="#ffedd5" stroke="#f97316"/>
  <text x="340" y="218" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">:socket_remote \u542f\u52a8</text>
  <text x="340" y="234" text-anchor="middle" font-size="10" fill="#64748b">onCreate - nativeStartSocketServer</text>
  <line x1="430" y1="222" x2="460" y2="222" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="460" y="200" width="180" height="44" rx="8" fill="#ffedd5" stroke="#f97316"/>
  <text x="550" y="218" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">SocketServer listen</text>
  <text x="550" y="234" text-anchor="middle" font-size="10" fill="#64748b">abstract:socketipc_calc</text>
  <line x1="640" y1="222" x2="670" y2="222" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="670" y="200" width="200" height="44" rx="8" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="770" y="218" text-anchor="middle" font-size="12" font-weight="600" fill="#1e40af">nativeWaitForSocketServer</text>
  <text x="770" y="234" text-anchor="middle" font-size="10" fill="#64748b">\u5faa\u73af ping() \u76f4\u5230\u5c31\u7eea (10s)</text>
  <line x1="870" y1="222" x2="900" y2="222" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="900" y="200" width="130" height="44" rx="8" fill="#dcfce7" stroke="#22c55e"/>
  <text x="965" y="218" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">UI \u5c31\u7eea</text>
  <text x="965" y="234" text-anchor="middle" font-size="10" fill="#64748b">\u53ef\u53d1\u8d77 IPC \u8c03\u7528</text>
  <path d="M 135 184 L 135 200" fill="none" stroke="#334155" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="70" y="258" width="1060" height="48" rx="8" fill="#fef9c3" stroke="#facc15"/>
  <text x="600" y="280" text-anchor="middle" font-size="11" fill="#854d0e">\u6ce8\u610f: \u5fc5\u987b\u7b49 Socket Server listen \u5b8c\u6210\u540e\u518d ping\uff0c\u5426\u5219 connect \u5931\u8d25</text>
  <text x="600" y="296" text-anchor="middle" font-size="11" fill="#854d0e">\u4fee\u590d\u524d: startService \u540e\u7acb\u5373 connect - C++ throw - JNI \u672a\u6355\u83b7 - SIGABRT \u5d29\u6e83</text>

  <rect x="40" y="340" width="540" height="320" rx="12" fill="#fff" stroke="#22c55e" stroke-width="1.5" filter="url(#sh)"/>
  <rect x="40" y="340" width="540" height="32" rx="12" fill="#16a34a"/>
  <rect x="40" y="360" width="540" height="12" fill="#16a34a"/>
  <text x="310" y="362" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">2. Binder IPC \u6d41\u7a0b (add \u793a\u4f8b)</text>
  <text x="130" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#1e40af">\u4e3b\u8fdb\u7a0b</text>
  <text x="310" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#64748b">Binder Driver</text>
  <text x="490" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#14532d">:binder_remote</text>
  <line x1="70" y1="404" x2="550" y2="404" stroke="#e2e8f0" stroke-width="1"/>
  <rect x="80" y="418" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="130" y="440" text-anchor="middle" font-size="11" fill="#1e40af">\u7528\u6237\u70b9\u51fb</text>
  <rect x="80" y="468" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="130" y="484" text-anchor="middle" font-size="11" fill="#1e40af">calculator</text>
  <text x="130" y="496" text-anchor="middle" font-size="9" fill="#64748b">.add(a,b)</text>
  <rect x="260" y="468" width="100" height="36" rx="6" fill="#f1f5f9" stroke="#94a3b8"/>
  <text x="310" y="490" text-anchor="middle" font-size="11" fill="#475569">\u5185\u6838\u8f6c\u53d1</text>
  <rect x="440" y="468" width="100" height="36" rx="6" fill="#dcfce7" stroke="#22c55e"/>
  <text x="490" y="484" text-anchor="middle" font-size="11" fill="#14532d">Stub.add</text>
  <text x="490" y="496" text-anchor="middle" font-size="9" fill="#64748b">a + b</text>
  <rect x="440" y="518" width="100" height="36" rx="6" fill="#dcfce7" stroke="#22c55e"/>
  <text x="490" y="540" text-anchor="middle" font-size="11" fill="#14532d">\u8fd4\u56de result</text>
  <rect x="260" y="568" width="100" height="36" rx="6" fill="#f1f5f9" stroke="#94a3b8"/>
  <text x="310" y="590" text-anchor="middle" font-size="11" fill="#475569">Parcel \u56de\u4f20</text>
  <rect x="80" y="618" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="130" y="640" text-anchor="middle" font-size="11" fill="#1e40af">UI \u663e\u793a\u7ed3\u679c</text>
  <line x1="130" y1="454" x2="130" y2="468" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <line x1="180" y1="486" x2="260" y2="486" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <line x1="360" y1="486" x2="440" y2="486" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <line x1="490" y1="504" x2="490" y2="518" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <line x1="440" y1="536" x2="360" y2="586" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <line x1="260" y1="586" x2="180" y2="636" stroke="#16a34a" stroke-width="2" marker-end="url(#arrGreen)"/>
  <rect x="70" y="670" width="500" height="70" rx="8" fill="#f0fdf4" stroke="#86efac"/>
  <text x="320" y="692" text-anchor="middle" font-size="12" font-weight="600" fill="#14532d">\u590d\u6742\u5bf9\u8c61 (Custom / Parcel)</text>
  <text x="320" y="710" text-anchor="middle" font-size="10" fill="#64748b">\u4e3b\u8fdb\u7a0b: NativeIpc.encode - ByteArray</text>
  <text x="320" y="726" text-anchor="middle" font-size="10" fill="#64748b">AIDL \u4f20\u8f93 - \u8fdc\u7a0b: NativeIpc.nativeProcess* - decode - \u5904\u7406 - encode - \u56de\u4f20</text>

  <rect x="620" y="340" width="540" height="320" rx="12" fill="#fff" stroke="#f97316" stroke-width="1.5" filter="url(#sh)"/>
  <rect x="620" y="340" width="540" height="32" rx="12" fill="#ea580c"/>
  <rect x="620" y="360" width="540" height="12" fill="#ea580c"/>
  <text x="890" y="362" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">3. Socket IPC \u6d41\u7a0b (add \u793a\u4f8b)</text>
  <text x="710" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#1e40af">\u4e3b\u8fdb\u7a0b</text>
  <text x="890" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#64748b">Unix Socket</text>
  <text x="1070" y="395" text-anchor="middle" font-size="11" font-weight="700" fill="#9a3412">:socket_remote</text>
  <line x1="650" y1="404" x2="1130" y2="404" stroke="#e2e8f0" stroke-width="1"/>
  <rect x="660" y="418" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="710" y="440" text-anchor="middle" font-size="11" fill="#1e40af">\u7528\u6237\u70b9\u51fb</text>
  <rect x="660" y="468" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="710" y="484" text-anchor="middle" font-size="11" fill="#1e40af">nativeSocketAdd</text>
  <text x="710" y="496" text-anchor="middle" font-size="9" fill="#64748b">JNI - C++</text>
  <rect x="660" y="518" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="710" y="534" text-anchor="middle" font-size="11" fill="#1e40af">SocketClient</text>
  <text x="710" y="546" text-anchor="middle" font-size="9" fill="#64748b">.add(a,b)</text>
  <rect x="840" y="518" width="100" height="36" rx="6" fill="#f1f5f9" stroke="#94a3b8"/>
  <text x="890" y="534" text-anchor="middle" font-size="11" fill="#475569">connect</text>
  <text x="890" y="546" text-anchor="middle" font-size="9" fill="#64748b">AF_UNIX</text>
  <rect x="1020" y="518" width="100" height="36" rx="6" fill="#ffedd5" stroke="#f97316"/>
  <text x="1070" y="534" text-anchor="middle" font-size="11" fill="#9a3412">handle_frame</text>
  <text x="1070" y="546" text-anchor="middle" font-size="9" fill="#64748b">kAdd - sum</text>
  <rect x="660" y="578" width="100" height="36" rx="6" fill="#dbeafe" stroke="#3b82f6"/>
  <text x="710" y="600" text-anchor="middle" font-size="11" fill="#1e40af">UI \u663e\u793a\u7ed3\u679c</text>
  <rect x="840" y="568" width="280" height="50" rx="8" fill="#fff7ed" stroke="#fdba74"/>
  <text x="980" y="588" text-anchor="middle" font-size="11" font-weight="600" fill="#9a3412">\u5e27\u683c\u5f0f: [opcode:1B][len:4B][payload:N]</text>
  <text x="980" y="606" text-anchor="middle" font-size="10" fill="#64748b">\u8bf7\u6c42 kAdd payload=8B(a+b) - \u54cd\u5e94 status+result</text>
  <line x1="710" y1="454" x2="710" y2="468" stroke="#ea580c" stroke-width="2" marker-end="url(#arrOrange)"/>
  <line x1="710" y1="504" x2="710" y2="518" stroke="#ea580c" stroke-width="2" marker-end="url(#arrOrange)"/>
  <line x1="760" y1="536" x2="840" y2="536" stroke="#ea580c" stroke-width="2" marker-end="url(#arrOrange)"/>
  <line x1="940" y1="536" x2="1020" y2="536" stroke="#ea580c" stroke-width="2" marker-end="url(#arrOrange)"/>
  <path d="M 1070 554 L 1070 596 L 760 596" fill="none" stroke="#ea580c" stroke-width="2" marker-end="url(#arrOrange)"/>
  <rect x="650" y="670" width="500" height="70" rx="8" fill="#fff7ed" stroke="#fdba74"/>
  <text x="900" y="692" text-anchor="middle" font-size="12" font-weight="600" fill="#9a3412">\u590d\u6742\u5bf9\u8c61 (Custom / Parcel)</text>
  <text x="900" y="710" text-anchor="middle" font-size="10" fill="#64748b">\u4e3b\u8fdb\u7a0b: CustomCodec/ParcelCodec.encode - SocketClient.process_*</text>
  <text x="900" y="726" text-anchor="middle" font-size="10" fill="#64748b">\u8fdc\u7a0b: kProcessCustom/kProcessParcel - decode - \u5904\u7406 - encode - Response \u56de\u4f20</text>

  <rect x="40" y="680" width="1120" height="200" rx="12" fill="#fff" stroke="#8b5cf6" stroke-width="1.5" filter="url(#sh)"/>
  <rect x="40" y="680" width="1120" height="32" rx="12" fill="#7c3aed"/>
  <rect x="40" y="700" width="1120" height="12" fill="#7c3aed"/>
  <text x="600" y="702" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">4. \u534f\u8bae\u4e0e\u538b\u6d4b\u6d41\u7a0b</text>
  <rect x="60" y="728" width="320" height="130" rx="8" fill="#faf5ff" stroke="#c4b5fd"/>
  <text x="220" y="750" text-anchor="middle" font-size="12" font-weight="700" fill="#5b21b6">Socket Opcode</text>
  <text x="80" y="772" font-size="11" fill="#334155">kPing = 4          \u5065\u5eb7\u68c0\u67e5</text>
  <text x="80" y="792" font-size="11" fill="#334155">kAdd = 1           \u6574\u6570\u52a0\u6cd5</text>
  <text x="80" y="812" font-size="11" fill="#334155">kProcessCustom = 2  CustomCodec \u5bf9\u8c61</text>
  <text x="80" y="832" font-size="11" fill="#334155">kProcessParcel = 3  ParcelCodec \u5bf9\u8c61</text>
  <rect x="400" y="728" width="320" height="130" rx="8" fill="#faf5ff" stroke="#c4b5fd"/>
  <text x="560" y="750" text-anchor="middle" font-size="12" font-weight="700" fill="#5b21b6">UserProfile \u590d\u6742\u5bf9\u8c61</text>
  <text x="420" y="772" font-size="11" fill="#334155">id / name / scores[] / rating / checksum</text>
  <text x="420" y="798" font-size="11" fill="#334155">CustomCodec: \u9b54\u6570 IPCC + \u7d27\u51d1\u4e8c\u8fdb\u5236</text>
  <text x="420" y="824" font-size="11" fill="#334155">ParcelCodec: \u6a21\u62df Android Parcel 4B \u5bf9\u9f50</text>
  <rect x="740" y="728" width="400" height="130" rx="8" fill="#faf5ff" stroke="#c4b5fd"/>
  <text x="940" y="750" text-anchor="middle" font-size="12" font-weight="700" fill="#5b21b6">\u591a\u7ebf\u7a0b\u538b\u6d4b\u6d41\u7a0b</text>
  <rect x="760" y="762" width="90" height="32" rx="6" fill="#ede9fe" stroke="#a78bfa"/>
  <text x="805" y="782" text-anchor="middle" font-size="10" fill="#5b21b6">\u914d\u7f6e\u7ebf\u7a0b/\u6b21\u6570</text>
  <line x1="850" y1="778" x2="870" y2="778" stroke="#7c3aed" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="870" y="762" width="90" height="32" rx="6" fill="#ede9fe" stroke="#a78bfa"/>
  <text x="915" y="782" text-anchor="middle" font-size="10" fill="#5b21b6">N x thread</text>
  <line x1="960" y1="778" x2="980" y2="778" stroke="#7c3aed" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="980" y="762" width="90" height="32" rx="6" fill="#ede9fe" stroke="#a78bfa"/>
  <text x="1025" y="782" text-anchor="middle" font-size="10" fill="#5b21b6">IPC \u8c03\u7528</text>
  <line x1="1070" y1="778" x2="1090" y2="778" stroke="#7c3aed" stroke-width="2" marker-end="url(#arr)"/>
  <rect x="1090" y="762" width="40" height="32" rx="6" fill="#ede9fe" stroke="#a78bfa"/>
  <text x="1110" y="782" text-anchor="middle" font-size="10" fill="#5b21b6">\u7edf\u8ba1</text>
  <text x="940" y="818" text-anchor="middle" font-size="10" fill="#64748b">Binder: BinderBenchmark - ICalculator.add / process*</text>
  <text x="940" y="836" text-anchor="middle" font-size="10" fill="#64748b">Socket: benchmark.cpp - SocketClient (C++ \u591a\u7ebf\u7a0b)</text>
  <text x="940" y="852" text-anchor="middle" font-size="10" fill="#64748b">\u8f93\u51fa: \u6210\u529f/\u5931\u8d25\u6b21\u6570 / \u5e73\u5747/\u6700\u5c0f/\u6700\u5927\u8017\u65f6 (ms)</text>

  <rect x="40" y="900" width="1120" height="170" rx="12" fill="#fff" stroke="#64748b" stroke-width="1.5" filter="url(#sh)"/>
  <rect x="40" y="900" width="1120" height="32" rx="12" fill="#475569"/>
  <rect x="40" y="920" width="1120" height="12" fill="#475569"/>
  <text x="600" y="922" text-anchor="middle" font-size="14" font-weight="700" fill="#fff">5. Binder vs Socket \u5bf9\u6bd4</text>
  <rect x="60" y="948" width="520" height="100" rx="8" fill="#f0fdf4" stroke="#86efac"/>
  <text x="320" y="972" text-anchor="middle" font-size="13" font-weight="700" fill="#14532d">Binder IPC</text>
  <text x="80" y="994" font-size="11" fill="#334155">+ Android \u539f\u751f IPC\uff0c\u7ecf Binder Driver \u5185\u6838\u8f6c\u53d1</text>
  <text x="80" y="1012" font-size="11" fill="#334155">+ AIDL \u81ea\u52a8\u751f\u6210 Stub/Proxy\uff0c\u7c7b\u578b\u5b89\u5168</text>
  <text x="80" y="1030" font-size="11" fill="#334155">+ Parcel \u5e8f\u5217\u5316\uff0c\u652f\u6301 Binder \u5bf9\u8c61\u4f20\u9012</text>
  <text x="80" y="1048" font-size="11" fill="#334155">- \u8de8\u8fdb\u7a0b\u8c03\u7528\u5f00\u9500\u76f8\u5bf9\u8f83\u5927\uff0c\u9700 Service \u7ed1\u5b9a</text>
  <rect x="620" y="948" width="520" height="100" rx="8" fill="#fff7ed" stroke="#fdba74"/>
  <text x="880" y="972" text-anchor="middle" font-size="13" font-weight="700" fill="#9a3412">Unix Domain Socket IPC</text>
  <text x="640" y="994" font-size="11" fill="#334155">+ \u7eaf\u7528\u6237\u6001 C++ \u5b9e\u73b0\uff0c\u81ea\u5b9a\u4e49\u4e8c\u8fdb\u5236\u534f\u8bae</text>
  <text x="640" y="1012" font-size="11" fill="#334155">+ \u540c UID \u8fdb\u7a0b\u95f4 abstract socket\uff0c\u65e0\u9700\u7f51\u7edc\u6743\u9650</text>
  <text x="640" y="1030" font-size="11" fill="#334155">+ \u7075\u6d3b\u53ef\u63a7\uff0c\u9002\u5408 Native \u5c42\u9ad8\u6027\u80fd\u901a\u4fe1</text>
  <text x="640" y="1048" font-size="11" fill="#334155">- \u9700\u81ea\u884c\u5904\u7406\u8fde\u63a5\u7ba1\u7406\u3001\u534f\u8bae\u3001\u9519\u8bef\u4e0e\u7ebf\u7a0b\u5b89\u5168</text>
</svg>"""

base = pathlib.Path(__file__).parent
(base / "architecture.svg").write_text(ARCH, encoding="utf-8")
(base / "flow.svg").write_text(FLOW, encoding="utf-8")
print("Done:", base / "architecture.svg", base / "flow.svg")
