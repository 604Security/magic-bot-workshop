// Sparkle test menu web page, served at "/". Plain HTML/CSS/JS, no build step.
#pragma once

const char PAGE_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sparkle's Potion Dungeon</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Fredoka:wght@400;600;700&family=Space+Mono:wght@700&family=Henny+Penny&display=swap" rel="stylesheet">
<style>
:root{
  --bg:#FFF6E9; --dot:#FFE3C2; --ink:#2B2D42; --muted:#6B6F80; --card:#FFFFFF; --line:#2B2D42;
  --yellow:#FFD166; --pink:#FF9EC4; --blue:#8FD3FF; --green:#9BE8B3; --purple:#C9B2FF; --orange:#FFB86B; --mint:#A8F0E6;
  --meter:#6C5CE7; --track:#EDE7F6; --glow:#FFB703; --unicorn:#FF5FA2; --unicorn-hi:#FFB3D4;
  --tile-ink:#2B2D42;
}
@media (prefers-color-scheme: dark){
  :root{
    --bg:#1C1B2E; --dot:#26243D; --ink:#F5F0FF; --muted:#B7B2D0; --card:#2A2842; --line:#0D0C18;
    --meter:#A99BFF; --track:#3A3757; --unicorn:#FF8CC0; --unicorn-hi:#FFC2DD;
  }
}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{
  margin:0; color:var(--ink); background-color:var(--bg);
  background-image:radial-gradient(var(--dot) 2px, transparent 2.5px); background-size:26px 26px;
  font-family:"Fredoka","Baloo 2","Comic Sans MS","Trebuchet MS",system-ui,sans-serif; font-size:18px;
}
main{max-width:780px; margin:0 auto; padding:0 16px 56px}
pre{font-family:"Space Mono",ui-monospace,Menlo,Consolas,monospace; margin:0}
a{color:inherit}

header{text-align:center; padding:22px 16px 6px}
.hero{display:inline-block; position:relative; max-width:100%}
.banner{font-size:clamp(10px,2.8vw,17px); line-height:1.12; color:var(--ink); text-align:left; overflow:hidden}
.tag{position:absolute; right:-10px; bottom:-8px; background:var(--pink); color:var(--tile-ink);
  border:3px solid var(--line); border-radius:999px; padding:1px 14px; font-weight:700; font-size:18px;
  transform:rotate(-7deg); box-shadow:3px 3px 0 var(--line)}

.panel[hidden]{display:none}
.greet{display:flex; align-items:center; justify-content:center; gap:18px; flex-wrap:wrap; margin:22px 0 26px}
.pet{font-size:15px; line-height:1.15; background:var(--card); border:3px solid var(--line); border-radius:22px;
  padding:12px 16px; box-shadow:5px 5px 0 var(--line); display:inline-block}
.pet.big{font-size:clamp(15px,4vw,22px)}
.ant{color:#FF4D6D; transition:color .1s, text-shadow .1s}
.ant.lit{color:var(--glow); text-shadow:0 0 10px var(--glow)}
.bubble{position:relative; background:var(--card); border:3px solid var(--line); border-radius:20px; padding:12px 18px;
  font-size:21px; box-shadow:4px 4px 0 var(--line); max-width:300px; font-weight:600}
.bubble small{display:block; font-weight:400; color:var(--muted); font-size:16px; margin-top:2px}

.tiles{display:grid; grid-template-columns:repeat(auto-fit,minmax(210px,1fr)); gap:18px}
.tile{display:flex; flex-direction:column; gap:2px; padding:18px 18px 16px; border:3px solid var(--line); border-radius:24px;
  box-shadow:6px 6px 0 var(--line); color:var(--tile-ink); text-decoration:none; transition:transform .12s, box-shadow .12s}
.tile:hover,.tile:focus-visible{transform:translate(-2px,-2px) rotate(-1.2deg); box-shadow:8px 8px 0 var(--line); outline:none}
.tile:active{transform:translate(4px,4px); box-shadow:2px 2px 0 var(--line)}
.tile .emo{font-size:42px; line-height:1.1}
.tile b{font-size:27px}
.tile small{font-size:17px; opacity:.85}
.t-yellow{background:var(--yellow)} .t-pink{background:var(--pink)} .t-blue{background:var(--blue)}
.t-green{background:var(--green)} .t-purple{background:var(--purple)} .t-mint{background:var(--mint)} .t-orange{background:var(--orange)}
.dancecount{font-size:26px; font-weight:700; margin-top:16px}
.owlsky{position:relative; height:70px; overflow:hidden; font-size:44px}
#owlbird{position:absolute; left:-120px; top:8px}
#owlbird.fly{animation:owlfly 3s ease-out forwards}
@keyframes owlfly{0%{left:-120px; top:20px} 25%{top:0} 50%{top:20px} 75%{top:0} 100%{left:calc(50% - 50px); top:10px}}
.letters{display:flex; flex-wrap:wrap; gap:12px; margin-top:6px}
.letters button{font-size:20px}
.snitchscore{font-size:72px; font-weight:700; margin:10px 0 0}
#snitchcatch{font-size:30px; padding:14px 30px}
.ingredients{display:flex; flex-wrap:wrap; gap:10px; margin:10px 0}
.ingredients button{font-size:19px}
.ingredients button.in{background:var(--green); color:var(--tile-ink)}
.pot{font-size:56px; margin:10px 0 0; min-height:70px}
#potin{font-size:36px}
.pot.brew #potemo{display:inline-block; animation:hatwob .15s ease-in-out infinite alternate}
#recipes{margin-top:18px; font-size:18px}
.hatbig{font-size:110px; line-height:1.1; display:inline-block}
.hatbig.think{animation:hatwob .3s ease-in-out infinite alternate}
@keyframes hatwob{from{transform:rotate(-8deg)} to{transform:rotate(8deg)}}
#hatgo{font-size:28px; padding:12px 26px}
.house{margin-top:18px; padding:18px 22px; border:3px solid var(--line); border-radius:22px; font-size:34px; font-weight:700;
  box-shadow:6px 6px 0 var(--line); display:inline-block}
.house small{display:block; font-size:20px; font-weight:500}
.status{text-align:center; color:var(--muted); margin-top:26px; font-size:17px}

.back{display:inline-block; margin:18px 0 4px; padding:6px 14px; border:3px solid var(--line); border-radius:999px;
  background:var(--card); text-decoration:none; font-weight:600; box-shadow:3px 3px 0 var(--line)}
.back:active{transform:translate(2px,2px); box-shadow:1px 1px 0 var(--line)}
h2{font-size:34px; margin:12px 0 4px}
.hint{font-size:20px; margin:0 0 18px; color:var(--muted)}

.btns{display:flex; flex-wrap:wrap; gap:12px; margin:18px 0}
button,.bigbtn{font:inherit; font-size:21px; font-weight:600; color:var(--tile-ink); background:var(--card); cursor:pointer;
  border:3px solid var(--line); border-radius:18px; padding:10px 18px; box-shadow:4px 4px 0 var(--line); transition:transform .08s, box-shadow .08s}
@media (prefers-color-scheme: dark){ button,.bigbtn{color:var(--ink)} }
button:hover{transform:translate(-1px,-1px); box-shadow:5px 5px 0 var(--line)}
button:active{transform:translate(3px,3px); box-shadow:1px 1px 0 var(--line)}
button.on{background:var(--yellow); color:var(--tile-ink)}
.bigbtn{font-size:24px; padding:14px 24px; background:var(--green); color:var(--tile-ink)}

.stage{display:flex; align-items:center; gap:22px; flex-wrap:wrap}
.bulb{width:120px; height:120px; border-radius:50%; border:4px solid var(--line); background:#6B5B3E;
  box-shadow:5px 5px 0 var(--line); transition:background .06s, box-shadow .06s}
.bulb.lit{background:var(--glow); box-shadow:5px 5px 0 var(--line), 0 0 40px 12px rgba(255,183,3,.7)}

.meter{height:38px; border:3px solid var(--line); border-radius:999px; background:var(--track); overflow:hidden; margin-top:20px}
.meter .fill{height:100%; width:0; background:var(--meter); border-radius:999px; transition:width .08s linear}
.ticks{display:flex; justify-content:space-between; color:var(--muted); font-size:15px; padding:4px 6px 0}
.stats{display:flex; gap:14px; align-items:stretch; margin-top:16px; flex-wrap:wrap}
.stat{background:var(--card); border:3px solid var(--line); border-radius:18px; padding:8px 18px; box-shadow:4px 4px 0 var(--line); min-width:130px}
.stat small{display:block; color:var(--muted); font-size:15px}
.stat b{font-size:34px; font-variant-numeric:tabular-nums}
.challenge{margin-top:20px; font-size:21px; font-weight:600; background:var(--card); border:3px dashed var(--line); border-radius:18px; padding:12px 16px}
.shake{animation:shake .25s linear infinite}
@keyframes shake{0%,100%{transform:rotate(0)}25%{transform:rotate(-3deg)}75%{transform:rotate(3deg)}}

.tv{background:var(--line); border:3px solid var(--line); border-radius:24px; padding:10px; box-shadow:6px 6px 0 var(--line); max-width:660px}
.tv img{display:block; width:100%; aspect-ratio:4/3; object-fit:cover; border-radius:16px; background:#111; transition:transform .3s}
.tv img.flip{transform:rotate(180deg)}
.photos{display:flex; flex-wrap:wrap; gap:12px}
.photos a{display:block; border:3px solid var(--line); border-radius:12px; overflow:hidden; box-shadow:3px 3px 0 var(--line); transform:rotate(-2deg)}
.photos a:nth-child(even){transform:rotate(2deg)}
.photos img{display:block; width:140px; aspect-ratio:4/3; object-fit:cover}

.result{margin-top:18px; background:var(--card); border:3px solid var(--line); border-radius:20px; padding:16px 18px; box-shadow:5px 5px 0 var(--line)}
.result h3{margin:0 0 6px; font-size:26px}
.chips{display:flex; flex-wrap:wrap; gap:8px; margin-top:10px}
.chip{background:var(--track); border-radius:999px; padding:3px 12px; font-size:15px}

.wifi{display:flex; align-items:center; gap:22px; background:var(--card); border:3px solid var(--line); border-radius:22px; padding:18px 22px; box-shadow:5px 5px 0 var(--line)}
.bars{display:flex; align-items:flex-end; gap:7px; height:80px}
.bars i{display:block; width:22px; border:3px solid var(--line); border-radius:6px; background:var(--track)}
.bars i:nth-child(1){height:25%} .bars i:nth-child(2){height:50%} .bars i:nth-child(3){height:75%} .bars i:nth-child(4){height:100%}
.bars i.on{background:var(--green)}
.wifi b{font-size:30px; display:block}
.facts{display:grid; grid-template-columns:max-content 1fr; gap:6px 16px; margin:18px 0 0; font-size:19px}
.facts dt{color:var(--muted)} .facts dd{margin:0; font-weight:600}

.oled{display:inline-block; background:#05060A; border:3px solid var(--line); border-radius:14px; padding:10px 12px; box-shadow:5px 5px 0 var(--line)}
.oled canvas{display:block; width:min(288px,70vw); height:auto; image-rendering:pixelated}
.unicorn{display:block; width:min(288px,70vw); height:auto; background:var(--card); border:3px solid var(--line);
  border-radius:22px; padding:12px; box-shadow:5px 5px 0 var(--line)}
.unicorn.small{width:120px; padding:8px; border-radius:18px}
.msgrow{display:flex; gap:10px; flex-wrap:wrap; margin-top:14px}
.msgrow input{font:inherit; font-size:22px; padding:10px 14px; border:3px solid var(--line); border-radius:16px; width:min(100%,280px);
  background:var(--card); color:var(--ink); box-shadow:4px 4px 0 var(--line)}
footer{text-align:center; color:var(--muted); font-size:15px; margin-top:34px}

/* ===== THEME: Potions Dungeon (Megaspy picked look 7 on pickers/05-wizard-makeover.html) =====
   Delete this block to go back to the old bright look. */
:root, :root:not(#x){
  --bg:#0d1410; --dot:#1a2a20; --ink:#c8f7d4; --muted:#8fc9a0; --card:#1b2a21; --line:#4d8a62;
  --yellow:#3b4a1e; --pink:#4a1f3a; --blue:#1d3346; --green:#1f4a2c; --purple:#33244d; --orange:#4a3018; --mint:#1d4440;
  --tile-ink:#d4ffe0; --meter:#3dff6e; --track:#1a2a20; --glow:#7dff9d; --unicorn:#7dff9d; --unicorn-hi:#c8ffd6;
  --magic:"Henny Penny","Fredoka",cursive;
}
body{background-color:var(--bg); background-image:radial-gradient(var(--dot) 2px, transparent 2.5px), linear-gradient(#1b2620,#0d1410);
  background-size:26px 26px, 100% 100%; background-attachment:fixed}
.banner{color:#7dff9d; text-shadow:0 0 8px #3dff6e}
.tag{background:#3dff6e; color:#0d1410; border-color:#0d1410}
h2,.tile b,.bubble,.house,.snitchscore,.dancecount{font-family:var(--magic); font-weight:400; color:#7dff9d; text-shadow:0 0 10px #3dff6e88}
.house{color:inherit; text-shadow:none}
.tile{box-shadow:0 0 0 0 transparent, 4px 4px 0 #050a07; border-color:#4d8a62}
.tile:hover,.tile:focus-visible{box-shadow:0 0 18px #3dff6e88, 6px 6px 0 #050a07}
.tile small{color:#a9e8b9}
button,.bigbtn,.back,.pet,.stat,.bubble{box-shadow:0 0 10px #3dff6e44, 3px 3px 0 #050a07}
button,.bigbtn{color:var(--tile-ink)}
.bubbles{position:fixed; inset:0; pointer-events:none; z-index:-1; overflow:hidden}
.bubbles span{position:absolute; bottom:-30px; border:2px solid #3dff6e66; border-radius:50%; animation:rise linear infinite}
@keyframes rise{from{transform:translateY(0); opacity:.9} to{transform:translateY(-110vh); opacity:0}}
</style>
</head>
<body>
<div class="bubbles" aria-hidden="true"><span style="left:5%;width:14px;height:14px;animation-duration:11s;animation-delay:-2s"></span><span style="left:14%;width:8px;height:8px;animation-duration:8s;animation-delay:-5s"></span><span style="left:27%;width:20px;height:20px;animation-duration:14s;animation-delay:-1s"></span><span style="left:41%;width:10px;height:10px;animation-duration:9s;animation-delay:-7s"></span><span style="left:55%;width:16px;height:16px;animation-duration:12s;animation-delay:-3s"></span><span style="left:68%;width:9px;height:9px;animation-duration:7s;animation-delay:-4s"></span><span style="left:79%;width:22px;height:22px;animation-duration:15s;animation-delay:-8s"></span><span style="left:90%;width:12px;height:12px;animation-duration:10s;animation-delay:-6s"></span></div>
<header>
  <div class="hero">
<pre class="banner" aria-label="Sparkle">  ____                   _    _
 / ___| _ __   __ _ _ __| | _| | ___
 \___ \| '_ \ / _` | '__| |/ / |/ _ \
  ___) | |_) | (_| | |  |   &lt;| |  __/
 |____/| .__/ \__,_|_|  |_|\_\_|\___|
       |_|</pre>
    <span class="tag">🧪 potions!</span>
  </div>
</header>

<main>
<!-- ============ HOME ============ -->
<section id="home" class="panel">
  <div class="greet">
    <pre class="pet" id="pet-home" aria-hidden="true"></pre>
    <canvas class="unicorn small" id="unicorn-home" width="144" height="160" aria-hidden="true"></canvas>
    <div class="bubble">Hi! I'm Sparkle.<small>What should we test today?</small></div>
  </div>
  <nav class="tiles">
    <a class="tile t-yellow" href="#lights"><span class="emo">💡</span><b>Lights</b><small>Make my light blink!</small></a>
    <a class="tile t-pink" href="#sounds"><span class="emo">🎤</span><b>Sounds</b><small>Clap and watch me hear you!</small></a>
    <a class="tile t-blue" href="#eyes"><span class="emo">📷</span><b>Eyes</b><small>See what I see!</small></a>
    <a class="tile t-purple" href="#screen"><span class="emo">🦄</span><b>Screen</b><small>Say hi to my unicorn!</small></a>
    <a class="tile t-green" href="#memory"><span class="emo">💾</span><b>Memory</b><small>Check my memory card</small></a>
    <a class="tile t-mint" href="#wifi"><span class="emo">📶</span><b>Wi-Fi</b><small>How strong is my signal?</small></a>
    <a class="tile t-blue" href="#owl"><span class="emo">🦉</span><b>Owl Post</b><small>Send a letter by owl!</small></a>
    <a class="tile t-yellow" href="#snitch"><span class="emo">🟡</span><b>Snitch</b><small>Catch the golden snitch!</small></a>
    <a class="tile t-purple" href="#hat"><span class="emo">🎩</span><b>Sorting Hat</b><small>Which house are you?</small></a>
    <a class="tile t-green" href="#potion"><span class="emo">🧪</span><b>Potions</b><small>Mix a silly potion!</small></a>
    <a class="tile t-blue" href="#wand"><span class="emo">🪄</span><b>Wand Wave</b><small>Wave me and cast a spell!</small></a>
    <a class="tile t-orange" href="#dance"><span class="emo">💃</span><b>Dance</b><small>Clap and my unicorn dances!</small></a>
  </nav>
  <p class="status" id="status">Waking Sparkle up…</p>
</section>

<!-- ============ LIGHTS ============ -->
<section id="lights" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>💡 Lights</h2>
  <p class="hint">Look for the tiny orange light on Sparkle, right next to the USB plug!</p>
  <div class="stage">
    <div class="bulb" id="bulb" aria-hidden="true"></div>
    <pre class="pet" id="pet-lights" aria-hidden="true"></pre>
  </div>
  <div class="btns" id="ledbtns">
    <button data-mode="on">☀️ On</button>
    <button data-mode="off">🌙 Off</button>
    <button data-mode="blink">✨ Blink</button>
    <button data-mode="disco">🪩 Disco</button>
    <button data-mode="hello">👋 Say “HI”</button>
  </div>
  <p class="hint" id="ledmsg">Press a button!</p>
</section>

<!-- ============ SOUNDS ============ -->
<section id="sounds" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🎤 Sounds</h2>
  <p class="hint">Clap, sing, or whisper. Sparkle is listening!</p>
  <div class="stage">
    <pre class="pet big" id="pet-sound" aria-hidden="true"></pre>
    <div class="bubble" id="soundword">zzZ… 😴</div>
  </div>
  <div class="meter" role="meter" aria-label="How loud" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" id="meter"><div class="fill" id="meterfill"></div></div>
  <div class="ticks"><span>shh</span><span>whisper</span><span>talk</span><span>CLAP!</span></div>
  <div class="stats">
    <div class="stat"><small>Now</small><b id="now">0</b></div>
    <div class="stat"><small>Loudest 🏆</small><b id="best">0</b></div>
    <button id="resetbest">↺ Reset</button>
  </div>
  <div class="challenge" id="challenge"></div>
</section>

<!-- ============ EYES ============ -->
<section id="eyes" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>📷 Eyes</h2>
  <p class="hint">This is what Sparkle sees right now. Wave hello! 👋</p>
  <div class="tv"><img id="cam" alt="What Sparkle's camera sees"></div>
  <div class="btns">
    <button id="snap">📸 Take a photo</button>
    <button id="flip">🙃 Flip</button>
  </div>
  <div class="photos" id="photos"></div>
</section>

<!-- ============ SCREEN ============ -->
<section id="screen" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🦄 Screen</h2>
  <p class="hint">Look at Sparkle's tiny screen. Is the unicorn there? It blinks and sparkles! (The real screen can only show white, so here she is in pink. 💖)</p>
  <div class="stage">
<canvas class="unicorn" id="unicorn-big" width="288" height="320" aria-label="Sparkle's pink unicorn"></canvas>
    <div class="oled" title="What Sparkle's real screen looks like"><canvas id="oled" width="288" height="160" aria-label="Sparkle's screen"></canvas></div>
  </div>
  <p class="hint" style="margin-top:18px">Write a message for the screen (up to 12 letters):</p>
  <div class="msgrow">
    <input id="msg" maxlength="12" placeholder="Your name!" autocomplete="off">
    <button id="sendmsg">✉️ Send</button>
    <button id="unicornbtn">🦄 Unicorn</button>
  </div>
  <p class="hint" id="screenmsg" style="margin-top:14px"></p>
</section>

<!-- ============ DANCE ============ -->
<section id="dance" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>💃 Unicorn Dance Party</h2>
  <p class="hint">Clap your hands loud! 👏 Sparkle's unicorn does the <b>Heart Twirl</b>. 💕</p>
  <div class="oled" title="What Sparkle's real screen looks like"><canvas id="danceoled" width="288" height="160" aria-label="Sparkle's screen"></canvas></div>
  <p style="margin-top:18px"><button id="dancebtn">💃 Dance now!</button></p>
  <p class="dancecount" id="dancecount"></p>
</section>

<!-- ============ OWL POST ============ -->
<section id="owl" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🦉 Owl Post</h2>
  <p class="hint">Pick a letter, and an owl flies it to Sparkle's screen! ✉️</p>
  <div class="owlsky"><span id="owlbird">🦉✉️</span></div>
  <div class="letters" id="letters"></div>
  <p class="hint" style="margin-top:18px">Or write your own letter (up to 24 letters):</p>
  <div class="msgrow">
    <input id="owlmsg" maxlength="24" placeholder="Hello Sparkle!" autocomplete="off">
    <button id="owlsend">🦉 Send</button>
  </div>
  <p class="dancecount" id="owlstatus"></p>
</section>

<!-- ============ SNITCH ============ -->
<section id="snitch" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🟡 Catch the Snitch</h2>
  <p class="hint">Watch Sparkle's screen! When the golden snitch flies into the box (it says <b>NOW!</b>), <b>CLAP</b>! 👏<br>
    Every catch makes it fly faster. ⚡</p>
  <p class="snitchscore"><span id="snitchscore">0</span> ⭐</p>
  <p class="hint" id="snitchbest"></p>
  <p style="margin-top:14px"><button id="snitchcatch">✋ Catch!</button> <button id="snitchnew">🔄 New game</button></p>
  <p class="dancecount" id="snitchmsg"></p>
</section>

<!-- ============ SORTING HAT ============ -->
<section id="hat" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🎩 The Sorting Hat</h2>
  <p class="hint"><b>Shake Sparkle</b> 🫨 (or clap 👏, or press the button) and the Sorting Hat will choose your house! Shake again to go back to the hat for the next person.</p>
  <div class="hatbig" id="hatbig">🎩</div>
  <p style="margin-top:10px"><button id="hatgo">🎩 Sort me!</button></p>
  <div class="house" id="house" hidden></div>
</section>

<!-- ============ POTION MIXER ============ -->
<section id="potion" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🧪 Potion Mixer</h2>
  <p class="hint">Pick <b>3 ingredients</b>, then <b>shake Sparkle</b> 🫨 to stir the cauldron (or press Stir). What will happen?!</p>
  <div class="ingredients" id="ingredients"></div>
  <div class="pot"><span id="potemo">⚗️</span> <span id="potin"></span></div>
  <p style="margin-top:10px"><button id="stir">🥄 Stir!</button> <button id="potempty">🔄 Empty</button></p>
  <div class="house" id="potresult" hidden></div>
  <div id="recipes"></div>
</section>

<!-- ============ WAND WAVE ============ -->
<section id="wand" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>🪄 Wand Wave</h2>
  <p class="hint"><b>Hold Sparkle like a wand and give it a big swish!</b> 🫴 It casts a spell. Watch its screen! (Or press Cast.)</p>
  <div class="hatbig" id="wandbig">🪄</div>
  <p style="margin-top:10px"><button id="wandcast">✨ Cast!</button></p>
  <div class="house" id="spell" hidden></div>
  <div id="spellbook"></div>
</section>

<!-- ============ MEMORY ============ -->
<section id="memory" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>💾 Memory</h2>
  <p class="hint">Sparkle keeps its sound recordings on a tiny memory card. Let's check it works!</p>
  <button class="bigbtn" id="sdgo">🔍 Check my memory card</button>
  <div class="result" id="sdresult" hidden></div>
</section>

<!-- ============ WIFI ============ -->
<section id="wifi" class="panel" hidden>
  <a class="back" href="#">← Menu</a>
  <h2>📶 Wi-Fi</h2>
  <p class="hint">Sparkle talks to this page over Wi-Fi. More green bars = stronger signal.</p>
  <div class="wifi">
    <div class="bars" id="bars"><i></i><i></i><i></i><i></i></div>
    <div><b id="wifiword">Checking…</b><span id="wifirssi" style="color:var(--muted)"></span></div>
  </div>
  <dl class="facts">
    <dt>Network</dt><dd id="wssid">…</dd>
    <dt>Sparkle's address</dt><dd id="wip">…</dd>
    <dt>Channel</dt><dd id="wch">…</dd>
    <dt>Awake for</dt><dd id="wup">…</dd>
  </dl>
</section>

<footer>🧪 Sparkle's Potion Dungeon · created by Megaspy 🦄🕵️</footer>
</main>

<script>
const $ = id => document.getElementById(id);
const api = path => fetch(path, {cache: 'no-store'}).then(r => r.json());

// ---------- the unicorn: the exact 36x40 bitmap Sparkle's OLED shows (from unicorn.py js) ----------
const UNICORN = ['000000000','000000000','000000010','000000030','000040060','0000400c0','000460040','001ff0380','003ff0700','007fd8f00','007fcdc00','00ffcf800','01ffffe00','01fff8f00','01fff0780','03dff4780','03dfe07e0','07fef07f8','07bdf07fc','07fcf8ffe','077cffffe','0ef8ffffe','0ef87ffe7','0df87ffff','0df9ffffe','1df1ffffe','1df1ffcee','1df1fef1c','1df1fe7f8','3de1fe0c0','3de1ff000','3de1ff000','3fe1ff000','7bc1ff000','7fc1ff000','77c3ff000','7fc3ff800','6f83ff800','6f83ff800','7fc3ff800'];
const UNICORN_BLINK = ['000000000','000000000','000000010','000000030','000040060','0000400c0','000460040','001ff0380','003ff0700','007fd8f00','007fcdc00','00ffcf800','01ffffe00','01fffff00','01fffff80','03dffff80','03dffffe0','07fef77f8','07bdf07fc','07fcfdffe','077cffffe','0ef8ffffe','0ef87ffe7','0df87ffff','0df9ffffe','1df1ffffe','1df1ffcee','1df1fef1c','1df1fe7f8','3de1fe0c0','3de1ff000','3de1ff000','3fe1ff000','7bc1ff000','7fc1ff000','77c3ff000','7fc3ff800','6f83ff800','6f83ff800','7fc3ff800'];
const UW = 36, UH = 40;
const toBits = rows => rows.map(h => parseInt(h, 16).toString(2).padStart(UW, '0'));
const U_OPEN = toBits(UNICORN), U_SHUT = toBits(UNICORN_BLINK);
const cssVar = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();

function sparkle(g, x, y, r, color) {
  g.fillStyle = color;
  g.fillRect(x - r, y - 1, 2 * r, 2);
  g.fillRect(x - 1, y - r, 2, 2 * r);
}
// Big friendly pixels in pink, with twinkling sparkles around the horn.
function drawPinkUnicorn(cv, t) {
  const g = cv.getContext('2d'), px = cv.width / UW, pink = cssVar('--unicorn'), hi = cssVar('--unicorn-hi');
  g.clearRect(0, 0, cv.width, cv.height);
  const rows = (t % 4000) < 160 ? U_SHUT : U_OPEN;
  rows.forEach((row, y) => [...row].forEach((b, x) => {
    if (b !== '1') return;
    g.fillStyle = pink;
    g.beginPath(); g.roundRect(x * px + 0.5, y * px + 0.5, px - 1, px - 1, px / 4); g.fill();
  }));
  // a soft highlight along the mane
  g.globalAlpha = 0.55; g.fillStyle = hi;
  for (let y = 10; y < UH; y += 3) g.fillRect((16 - y / 3.2) * px, y * px, px * 0.6, px * 1.6);
  g.globalAlpha = 1;
  const k = Math.floor(t / 300) % 4, gold = cssVar('--glow');
  sparkle(g, 26 * px, 3 * px, k === 0 ? px * 1.6 : px * 0.9, gold);
  if (k === 2) sparkle(g, 34 * px, 9 * px, px * 1.4, pink);
  if (k === 1) sparkle(g, 6 * px, 5 * px, px * 1.2, gold);
}
// A look-alike of Sparkle's real 72x40 white-on-black screen.
function drawOled(cv, t, text) {
  const g = cv.getContext('2d'), px = cv.width / 72;
  g.fillStyle = '#05060A'; g.fillRect(0, 0, cv.width, cv.height);
  g.fillStyle = '#EAF6FF';
  const rows = (t % 4000) < 160 ? U_SHUT : U_OPEN;
  rows.forEach((row, y) => [...row].forEach((b, x) => { if (b === '1') g.fillRect(x * px, y * px, px - 0.6, px - 0.6); }));
  const k = Math.floor(t / 300) % 4;
  sparkle(g, 26.5 * px, 3.5 * px, k === 0 ? px * 2.2 : px * 1.2, '#EAF6FF');
  g.font = 'bold ' + (px * 10) + 'px "Space Mono", monospace';
  g.textAlign = 'center'; g.textBaseline = 'alphabetic';
  if (text === null) {
    g.fillText('Hi!', 54 * px, 12 * px);
    g.font = (px * 7) + 'px "Space Mono", monospace';
    g.fillText('Sparkle', 54 * px, 22 * px);
    const r = (Math.floor(t / 400) % 2 ? 2 : 1.5) * px, cx = 54 * px, cy = 28 * px;
    g.beginPath(); g.arc(cx - r, cy, r, 0, 7); g.arc(cx + r, cy, r, 0, 7); g.fill();
    g.beginPath(); g.moveTo(cx - 2 * r - px, cy + px); g.lineTo(cx + 2 * r + px, cy + px); g.lineTo(cx, cy + 2 * r + 3 * px); g.fill();
  } else {
    g.font = (px * 9) + 'px "Space Mono", monospace';
    g.fillText(text.slice(0, 6), 54 * px, 14 * px);
    g.fillText(text.slice(6, 12), 54 * px, 26 * px);
  }
}

// ---------- the ASCII robot pet ----------
// eye = 3 chars, mouth = 5 chars
function pet(eye, mouth, antLit) {
  return [
    '         <span class="ant' + (antLit ? ' lit' : '') + '">*</span>',
    '   /\\    |    /\\',
    '  /  \\_______/  \\',
    ' |               |',
    ' |  ' + eye + '     ' + eye + '  |',
    ' |       ^       |',
    ' |     ' + mouth + '     |',
    '  \\_____________/',
    '    |_|     |_|'
  ].join('\n');
}
const FACE = {
  happy:  ['(^)', '\\___/'],
  sleep:  ['(-)', '  o  '],
  listen: ['(o)', ' --- '],
  yay:    ['(O)', '\\___/'],
  wow:    ['(@)', ' (O) '],
  sad:    ['(;)', ' /-\\ ']
};

// ---------- router: each test has its own link, e.g. /#sounds ----------
const PANELS = ['home', 'lights', 'sounds', 'eyes', 'screen', 'memory', 'wifi', 'dance', 'owl', 'snitch', 'hat', 'potion', 'wand'];
let timers = [];
function every(ms, fn) { fn(); timers.push(setInterval(fn, ms)); }
function stopAll() {
  timers.forEach(clearInterval); timers = [];
  $('cam').removeAttribute('src');
}
function route() {
  const want = location.hash.slice(1) || 'home';
  const cur = PANELS.includes(want) ? want : 'home';
  PANELS.forEach(p => $(p).hidden = p !== cur);
  stopAll();
  START[cur]();
  if (cur !== 'screen') fetch('/api/screen?view=' + cur).catch(() => {});
  window.scrollTo(0, 0);
}
addEventListener('hashchange', route);

// ---------- home ----------
function startHome() {
  let t = 0;
  every(200, () => {
    t++;
    const blink = t % 18 === 0;
    $('pet-home').innerHTML = pet(blink ? '(-)' : '(^)', '\\___/', t % 5 < 2);
    drawPinkUnicorn($('unicorn-home'), performance.now());
  });
  api('/api/info').then(i => {
    const ok = v => v ? '✅' : '❌';
    $('status').textContent = `📶 Wi-Fi ${ok(true)}  ·  📷 camera ${ok(i.camera)}  ·  🎤 mic ${ok(i.mic)}  ·  🦄 screen ${ok(i.screen)}`;
  }).catch(() => $('status').textContent = "Hmm, I can't reach Sparkle. Is it switched on?");
}

// ---------- lights ----------
const HELLO = [1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 7], UNIT = 150;
const LED_MSG = {
  on: 'The light is ON. ☀️', off: 'The light is off. Night night! 🌙', blink: 'Blink… blink… blink… ✨',
  disco: 'DISCO PARTY! 🪩🕺💃', hello: 'Sparkle is saying “HI” in Morse code: •••• ••'
};
let ledMode = 'off', ledT0 = 0;
function ledState(t) {
  switch (ledMode) {
    case 'on': return true;
    case 'blink': return Math.floor(t / 500) % 2 === 0;
    case 'disco': return Math.random() < 0.5;
    case 'hello': {
      const total = HELLO.reduce((a, b) => a + b) * UNIT;
      let x = t % total, i = 0;
      while (x >= HELLO[i] * UNIT) { x -= HELLO[i] * UNIT; i++; }
      return i % 2 === 0;
    }
    default: return false;
  }
}
function showLedMode(mode) {
  ledMode = mode; ledT0 = performance.now();
  document.querySelectorAll('#ledbtns button').forEach(b => b.classList.toggle('on', b.dataset.mode === mode));
  $('ledmsg').textContent = LED_MSG[mode] || '';
}
document.querySelectorAll('#ledbtns button').forEach(b => b.onclick = () =>
  api('/api/led?mode=' + b.dataset.mode).then(r => showLedMode(r.mode)));
function startLights() {
  api('/api/info').then(i => showLedMode(i.led));
  every(60, () => {
    const lit = ledState(performance.now() - ledT0);
    $('bulb').classList.toggle('lit', lit);
    $('pet-lights').innerHTML = pet(lit ? '(O)' : '(o)', lit ? '\\___/' : ' --- ', lit);
  });
}

// ---------- sounds ----------
const QUIET = 40, TALK = 250, LOUD = 1200;   // RMS levels; quiet room is ~10-25
let shown = 0, best = 0, inFlight = false, zone = '', zoneSince = 0, step = 0;
const CHALLENGES = [
  {text: '🎯 Challenge: can you make Sparkle say WOW?', done: z => z === 'wow'},
  {text: '🤫 Now be super quiet so Sparkle falls asleep… (3 seconds)', done: (z, ms) => z === 'sleep' && ms > 3000},
  {text: '🗣️ Talk to Sparkle in your normal voice for 2 seconds', done: (z, ms) => (z === 'listen' || z === 'yay') && ms > 2000},
  {text: '🎵 Sing a song! Keep Sparkle saying “Yay!”', done: (z, ms) => z === 'yay' && ms > 1500},
];
function loud01(level) { return Math.min(1, Math.max(0, (Math.log10(Math.max(level, 10)) - 1) / 2.8)); }
function showChallenge() { $('challenge').textContent = CHALLENGES[step].text; }
$('resetbest').onclick = () => { best = 0; $('best').textContent = 0; };
function startSounds() {
  showChallenge();
  every(100, () => {
    if (inFlight) return;
    inFlight = true;
    api('/api/sound').then(s => {
      const lvl = s.peak;
      shown = Math.max(lvl, shown * 0.8);
      if (lvl > best) { best = lvl; $('best').textContent = best; }
      $('now').textContent = Math.round(shown);
      const pct = Math.round(loud01(shown) * 100);
      $('meterfill').style.width = pct + '%';
      $('meter').setAttribute('aria-valuenow', pct);

      const z = shown < QUIET ? 'sleep' : shown < TALK ? 'listen' : shown < LOUD ? 'yay' : 'wow';
      const now = performance.now();
      if (z !== zone) { zone = z; zoneSince = now; }
      const [eye, mouth] = FACE[z];
      $('pet-sound').innerHTML = pet(eye, mouth, z !== 'sleep');
      $('pet-sound').classList.toggle('shake', z === 'wow');
      $('soundword').textContent = {sleep: 'zzZ… 😴', listen: 'I hear you! 👂', yay: 'Yay! 😄', wow: 'WOW!!! 🤩'}[z];

      if (CHALLENGES[step].done(z, now - zoneSince)) {
        $('challenge').textContent = '🎉 You did it! 🌟';
        step = (step + 1) % CHALLENGES.length;
        setTimeout(showChallenge, 2000);
      }
    }).catch(() => {}).finally(() => inFlight = false);
  });
}

// ---------- eyes ----------
$('flip').onclick = () => $('cam').classList.toggle('flip');
$('snap').onclick = () => fetch('/capture', {cache: 'no-store'}).then(r => r.blob()).then(b => {
  const url = URL.createObjectURL(b);
  const a = document.createElement('a');
  a.href = url; a.download = 'sparkle-photo.jpg'; a.title = 'Click to save';
  a.innerHTML = '<img alt="Sparkle photo" src="' + url + '">';
  if ($('cam').classList.contains('flip')) a.firstChild.style.transform = 'rotate(180deg)';
  $('photos').prepend(a);
});
function startEyes() {
  $('cam').src = 'http://' + location.hostname + ':81/stream';
}

// ---------- screen ----------
function sendScreen(q, label) {
  api('/api/screen?' + q).then(r => {
    $('screenmsg').textContent = r.screen ? label : "I can't find Sparkle's screen. Is the screen board plugged in?";
  }).catch(() => {});
}
let oledText = null;  // null = unicorn says "Hi!"
$('sendmsg').onclick = () => {
  const m = $('msg').value.trim().slice(0, 12);
  if (!m) return;
  oledText = m;
  sendScreen('msg=' + encodeURIComponent(m), '✉️ Look at the screen! Your message is there.');
};
$('msg').addEventListener('keydown', e => { if (e.key === 'Enter') $('sendmsg').click(); });
$('unicornbtn').onclick = () => {
  oledText = null;
  sendScreen('view=home', '🦄 The unicorn says hi!');
};
function startScreen() {
  $('unicornbtn').click();
  every(100, () => {
    const t = performance.now();
    drawPinkUnicorn($('unicorn-big'), t);
    drawOled($('oled'), t, oledText);
  });
}

// ---------- memory ----------
$('sdgo').onclick = () => {
  const box = $('sdresult');
  box.hidden = false;
  box.innerHTML = '<h3>🔍 Looking…</h3>';
  api('/api/sd').then(r => {
    if (!r.type) {
      box.innerHTML = '<h3>😟 No card</h3><p>' + (r.message || '') + '</p>';
      return;
    }
    const gb = (r.sizeMB / 1024).toFixed(1);
    const chips = r.files.map(f => '<span class="chip">' + f.replace(/</g, '&lt;') + '</span>').join('');
    box.innerHTML = (r.ok ? '<h3>✅ My memory works!</h3>' : '<h3>😟 Something went wrong</h3>') +
      '<p>I found a <b>' + gb + ' GB</b> ' + r.type + ' card.</p>' +
      '<p>I wrote a secret note and read it back:<br><b>“' + r.note.replace(/</g, '&lt;') + '”</b></p>' +
      '<p>' + r.count + ' thing' + (r.count === 1 ? '' : 's') + ' on the card:</p><div class="chips">' + chips + '</div>';
  }).catch(() => box.innerHTML = "<h3>😟 Oops</h3><p>I couldn't reach Sparkle.</p>");
};
function startMemory() { $('sdresult').hidden = true; }

// ---------- wifi ----------
function startWifi() {
  every(2000, () => api('/api/info').then(i => {
    const bars = i.rssi > -55 ? 4 : i.rssi > -67 ? 3 : i.rssi > -75 ? 2 : i.rssi > -85 ? 1 : 0;
    document.querySelectorAll('#bars i').forEach((b, n) => b.classList.toggle('on', n < bars));
    $('wifiword').textContent = ['Very weak 😟', 'Weak 🙁', 'Okay 🙂', 'Good 😃', 'Super strong! 💪'][bars];
    $('wifirssi').textContent = i.rssi + ' dBm';
    $('wssid').textContent = i.ssid;
    $('wip').textContent = i.ip;
    $('wch').textContent = i.channel + ' (2.4 GHz)';
    const m = Math.floor(i.uptime / 60);
    $('wup').textContent = m ? m + ' min ' + (i.uptime % 60) + ' s' : i.uptime + ' s';
  }).catch(() => {}));
}

// ---------- dance: the same Heart Twirl the real screen draws ----------
function drawDance(cv, t) {
  const g = cv.getContext('2d'), s = cv.width / 72;
  g.fillStyle = '#05060A'; g.fillRect(0, 0, cv.width, cv.height);
  g.fillStyle = '#EAF6FF';
  const dot = (x, y) => { if (x >= 0 && x < 72 && y >= 0 && y < 40) g.fillRect(x * s, y * s, s - 0.6, s - 0.6); };
  const unicorn = (x0, flip) => U_OPEN.forEach((row, y) => [...row].forEach((b, x) => { if (b === '1') dot(x0 + (flip ? 35 - x : x), y); }));
  const heart = (x, y, r) => {
    for (let dy = -r; dy <= 2 * r + 2; dy++) for (let dx = -2 * r - 1; dx <= 2 * r + 1; dx++) {
      const inT = dy > 0 && Math.abs(dx) <= (2 * r + 1) * (1 - dy / (2 * r + 3));
      if ((dx + r) ** 2 + dy ** 2 <= r * r + 1 || (dx - r) ** 2 + dy ** 2 <= r * r + 1 || inT) dot(x + dx, y + dy);
    }
  };
  if (t === null) {  // waiting for a clap
    unicorn(0, false);
    g.font = (s * 9) + 'px "Space Mono", monospace'; g.textAlign = 'center';
    g.fillText('clap', 54 * s, 13 * s); g.fillText('me!', 54 * s, 25 * s);
    return;
  }
  unicorn(4 + Math.round(Math.sin(t / 1000 * 1.5 * 2 * Math.PI) * 3), Math.floor(t / 750) % 2 === 1);
  const HEARTS = 10;
  for (let i = 0; i < HEARTS; i++) {
    const period = 1200 + (i * 370) % 1000, p = ((t / period) + i / HEARTS) % 1;
    heart(41 + (i * 11) % 28 + Math.round(Math.sin(p * 4 * Math.PI + i) * 2), 44 - Math.floor(p * 50), i % 3 === 0 ? 2 : 1);
  }
}
let danceT0 = 0, danceLeft = 0;
function showDance(r) {
  if (r.left > 0 && danceLeft === 0) danceT0 = performance.now() - (r.total - r.left);
  danceLeft = r.left;
  $('dancecount').textContent = r.count ? `🦄 Sparkle has danced ${r.count} time${r.count === 1 ? '' : 's'}!` : '';
}
$('dancebtn').onclick = () => api('/api/dance?go=1').then(showDance).catch(() => {});
function startDance() {
  every(250, () => api('/api/dance').then(showDance).catch(() => {}));
  every(50, () => {
    const t = performance.now() - danceT0;
    drawDance($('danceoled'), danceLeft > 0 ? t : null);
  });
}

// ---------- owl post ----------
const LETTERS = [['💖', 'I LOVE YOU!'], ['🦉', 'HOOT HOOT!'], ['👋', 'HELLO SPARKLE'], ['✨', 'YOU ARE MAGIC'],
                 ['🦄', 'UNICORNS RULE'], ['🍪', 'WANT A COOKIE?'], ['🌙', 'NIGHT NIGHT'], ['🕵️', 'TOP SECRET!']];
function sendOwl(msg) {
  const bird = $('owlbird');
  bird.classList.remove('fly'); void bird.offsetWidth; bird.classList.add('fly');
  $('owlstatus').textContent = '🦉 The owl is flying…';
  api('/api/owl?msg=' + encodeURIComponent(msg)).then(r => setTimeout(() =>
    $('owlstatus').textContent = '📬 Delivered! Look at Sparkle\'s screen!', r.fly + 1500)).catch(() =>
    $('owlstatus').textContent = "😟 The owl got lost. Is Sparkle switched on?");
}
LETTERS.forEach(([emo, msg]) => {
  const b = document.createElement('button'); b.textContent = emo + ' ' + msg; b.onclick = () => sendOwl(msg);
  $('letters').appendChild(b);
});
$('owlsend').onclick = () => { const m = $('owlmsg').value.trim(); if (m) sendOwl(m); };
function startOwl() { $('owlstatus').textContent = ''; $('owlbird').classList.remove('fly'); }

// ---------- catch the snitch ----------
let snitchLast = -1;
const CHEERS = ['⚡ GOT IT!', '🧹 Amazing catch!', '🏆 You are a Seeker!', '✨ Wow, so fast!', '🟡 Gotcha!'];
function showSnitch(r) {
  if (snitchLast >= 0 && r.score > snitchLast) $('snitchmsg').textContent = CHEERS[r.score % CHEERS.length];
  if (r.score === 0) $('snitchmsg').textContent = '';
  snitchLast = r.score;
  $('snitchscore').textContent = r.score;
  $('snitchbest').textContent = r.best ? `🏆 Best score: ${r.best}` : '';
}
$('snitchcatch').onclick = () => api('/api/snitch?catch=1').then(() => setTimeout(() => api('/api/snitch').then(showSnitch), 100)).catch(() => {});
$('snitchnew').onclick = () => api('/api/snitch?reset=1').then(showSnitch).catch(() => {});
function startSnitch() { snitchLast = -1; every(300, () => api('/api/snitch').then(showSnitch).catch(() => {})); }

// ---------- sorting hat ----------
const HOUSES = [
  {name: 'GRYFFINDOR!', emo: '🦁', bg: '#B3202A', fg: '#F5C518', say: 'Brave and bold, like a lion!'},
  {name: 'RAVENCLAW!', emo: '🦅', bg: '#1E3A8A', fg: '#C9A66B', say: 'Clever and wise, like an eagle!'},
  {name: 'HUFFLEPUFF!', emo: '🦡', bg: '#F2C230', fg: '#222', say: 'Kind and loyal, like a badger!'},
  {name: 'SLYTHERIN!', emo: '🐍', bg: '#1F6B3A', fg: '#D9D9D9', say: 'Clever and sneaky, like a snake!'},
];
let hatShown = -1;
function hatSay(text) { try { speechSynthesis.cancel(); const u = new SpeechSynthesisUtterance(text); u.pitch = 0.8; u.rate = 0.9; speechSynthesis.speak(u); } catch (e) {} }
function showHat(r) {
  $('hatbig').classList.toggle('think', r.state === 'thinking');
  if (r.state === 'waiting') { $('house').hidden = true; hatShown = -1; }
  if (r.state === 'thinking') { $('house').hidden = true; if (hatShown !== -2) hatSay('Hmmm… tricky… very tricky…'); hatShown = -2; }
  if (r.state === 'done' && hatShown !== r.start) {
    const h = HOUSES[r.house], first = hatShown === -2;
    hatShown = r.start;
    $('house').hidden = false; $('house').style.background = h.bg; $('house').style.color = h.fg;
    $('house').innerHTML = `${h.emo} ${h.name} ${h.emo}<small>${h.say}</small>`;
    if (first) hatSay(h.name + ' ' + h.say);
  }
}
$('hatgo').onclick = () => api('/api/hat?go=1').then(showHat).catch(() => {});
function startHat() { hatShown = -1; every(300, () => api('/api/hat').then(showHat).catch(() => {})); }

// ---------- potion mixer ----------
const INGREDIENTS = [['🦄', 'unicorn hair'], ['🐸', 'frog spit'], ['🌈', 'rainbow drops'], ['🕷️', 'spider leg'],
                     ['🍬', 'candy'], ['🌶️', 'dragon pepper'], ['🪶', 'owl feather'], ['⭐', 'star dust']];
const POTIONS = [['🐱', "MEOW! YOU'RE A CAT!"], ['💗', 'PINK HAIR!'], ['🕊️', 'YOU CAN FLY!'], ['🫧', 'BUBBLE BURPS!'],
                 ['🐸', 'RIBBIT! FROG VOICE'], ['👻', "YOU'RE INVISIBLE!"], ['🌟', 'YOU GLOW LIKE A STAR'],
                 ['🦄', 'UNICORN HORN!'], ['🤧', 'SUPER SNEEZE! ACHOO'], ['🐰', 'GIANT BUNNY EARS!'],
                 ['🐉', 'YOU BREATHE FIRE!'], ['🍭', 'EVERYTHING TASTES SWEET']];
let potPicks = [], potShown = 0;
// the same 3 ingredients always make the same potion, so you can discover them all
const potionFor = picks => POTIONS[[...picks].sort((a, b) => a - b).reduce((h, i) => h * 11 + i + 1, 0) % POTIONS.length];
function potRender() {
  document.querySelectorAll('#ingredients button').forEach((b, i) => b.classList.toggle('in', potPicks.includes(i)));
  $('potin').textContent = potPicks.map(i => INGREDIENTS[i][0]).join(' ');
}
function loadRecipes() { try { return JSON.parse(localStorage.getItem('potions') || '[]'); } catch (e) { return []; } }
function showRecipes() {
  const r = loadRecipes();
  $('recipes').innerHTML = r.length ? `📖 <b>Your potion book (${r.length} of ${POTIONS.length} found):</b> ` + r.join(' · ') : '';
}
INGREDIENTS.forEach(([emo, name], i) => {
  const b = document.createElement('button'); b.textContent = emo + ' ' + name;
  b.onclick = () => {
    if (potPicks.includes(i)) potPicks = potPicks.filter(x => x !== i);
    else if (potPicks.length < 3) potPicks.push(i);
    potRender();
    let q = '/api/potion?ing=' + potPicks.join(',');
    if (potPicks.length === 3) q += '&msg=' + encodeURIComponent(potionFor(potPicks)[1]);
    api(q).catch(() => {});
  };
  $('ingredients').appendChild(b);
});
$('stir').onclick = () => api('/api/potion?stir=1').catch(() => {});
$('potempty').onclick = () => { potPicks = []; potRender(); api('/api/potion?empty=1').catch(() => {}); };
function showPotion(r) {
  $('potemo').parentElement.classList.toggle('brew', r.state === 2);
  if (r.state === 0 && potShown === 3) { potPicks = []; potRender(); }
  if (r.state === 3 && potShown !== 3 && potPicks.length === 3) {
    const [emo, text] = potionFor(potPicks);
    $('potresult').hidden = false; $('potresult').style.background = 'var(--purple)'; $('potresult').style.color = 'var(--tile-ink)';
    $('potresult').innerHTML = `${emo} ${text} ${emo}`;
    hatSay(text.replace("'", ''));
    const book = loadRecipes(), entry = emo + ' ' + text;
    if (!book.includes(entry)) { book.push(entry); try { localStorage.setItem('potions', JSON.stringify(book)); } catch (e) {} }
    showRecipes();
  }
  if (r.state !== 3) $('potresult').hidden = true;
  potShown = r.state;
}
function startPotion() { potRender(); showRecipes(); every(300, () => api('/api/potion').then(showPotion).catch(() => {})); }

// ---------- wand wave ----------
const SPELLS = [['💡', 'LUMOS!', 'Light up! Look at Sparkle\'s little light!'], ['⭐', 'ACCIO!', 'Here comes a star!'],
                ['🪶', 'LEVIOSA!', 'The feather floats!'], ['⚡', 'EXPELLIARMUS!', 'Sparks fly!'], ['💥', 'STUPEFY!', 'Flash!']];
let wandSeen = -1, spellsCast = 0;
function showWand(r) {
  if (r.spell < 0 || r.at === wandSeen) return;
  const first = wandSeen !== -1;
  wandSeen = r.at;
  if (!first) return;  // don't replay an old spell when the page opens
  const [emo, name, say] = SPELLS[r.spell];
  spellsCast++;
  $('wandbig').classList.remove('think'); void $('wandbig').offsetWidth; $('wandbig').classList.add('think');
  setTimeout(() => $('wandbig').classList.remove('think'), 1200);
  $('spell').hidden = false; $('spell').style.background = 'var(--purple)'; $('spell').style.color = 'var(--tile-ink)';
  $('spell').innerHTML = `${emo} ${name} ${emo}<small>${say}</small>`;
  $('spellbook').innerHTML = `<p class="dancecount">✨ Spells cast: ${spellsCast}</p>`;
  hatSay(name);
}
$('wandcast').onclick = () => api('/api/wand?cast=1').catch(() => {});
function startWand() { wandSeen = -1; every(250, () => api('/api/wand').then(r => { if (wandSeen === -1) wandSeen = r.at || 0; showWand(r); }).catch(() => {})); }

const START = {home: startHome, lights: startLights, sounds: startSounds, eyes: startEyes,
               screen: startScreen, memory: startMemory, wifi: startWifi, dance: startDance, owl: startOwl, snitch: startSnitch, hat: startHat, potion: startPotion, wand: startWand};
route();
</script>
</body>
</html>
)rawliteral";
