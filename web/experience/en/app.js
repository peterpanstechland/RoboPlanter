// A browser-only demonstration. No Bluetooth, HTTP device calls or printer I/O.
const $=id=>document.getElementById(id);
let fill=false;
$('light-toggle').addEventListener('click',()=>{fill=!fill;$('mini-device').classList.toggle('fill',fill);$('light-toggle').setAttribute('aria-pressed',String(fill));$('light-toggle').textContent=fill?'Turn fill light off':'Turn fill light on';$('light-status').textContent=fill?'WHITE / FILL-LIGHT DEMO':'GREEN / ONLINE STATUS DEMO';$('demo-message').textContent=fill?'A little light for your leaves. Demo only.':'Back to a little green. Demo only.';});
let remaining=25*60*1000,deadline=0,running=false;
function updateFocus(){if(running){remaining=Math.max(0,deadline-performance.now());if(!remaining){running=false;$('focus-status').textContent='Session complete. Take a small break. Browser demo only.';}}const s=Math.ceil(remaining/1000);$('focus-time').textContent=String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0');$('focus-toggle').textContent=running?'Pause focus':remaining===25*60*1000?'Start focus':remaining?'Resume focus':'Start again';$('focus-toggle').setAttribute('aria-pressed',String(running));}
$('focus-toggle').addEventListener('click',()=>{if(running){remaining=Math.max(0,deadline-performance.now());running=false;}else{if(!remaining)remaining=25*60*1000;deadline=performance.now()+remaining;running=true;}$('focus-status').textContent=running?'One small thing at a time. This timer runs only in this page.':'Paused. Take your time.';updateFocus();});
$('focus-reset').addEventListener('click',()=>{remaining=25*60*1000;running=false;$('focus-status').textContent="A browser timer; the real device uses XIAO's shared timer.";updateFocus();});
setInterval(updateFocus,250);
const major=['The Fool','The Magician','The High Priestess','The Empress','The Emperor','The Hierophant','The Lovers','The Chariot','Strength','The Hermit','Wheel of Fortune','Justice','The Hanged Man','Death','Temperance','The Devil','The Tower','The Star','The Moon','The Sun','Judgement','The World'];
const ranks=['Ace','Two','Three','Four','Five','Six','Seven','Eight','Nine','Ten','Page','Knight','Queen','King'];
const deck=[...major,...['Wands','Cups','Swords','Pentacles'].flatMap(s=>ranks.map(r=>`${r} of ${s}`))];
function draw(){const random=new Uint32Array(2);crypto.getRandomValues(random);$('tarot-card').textContent=deck[random[0]%deck.length];const reversed=Boolean(random[1]&1);$('tarot-orientation').textContent=reversed?'Reversed · a moment to reflect':'Upright · a small step forward';$('tarot-symbol').style.transform=reversed?'rotate(180deg)':'none';}
$('ticket-open').addEventListener('click',()=>{draw();$('ticket-dialog').showModal();});
$('ticket-close').addEventListener('click',()=>$('ticket-dialog').close());
$('ticket-redraw').addEventListener('click',draw);
$('ticket-dialog').addEventListener('click',e=>{if(e.target===$('ticket-dialog')){const r=e.target.getBoundingClientRect();if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)e.target.close();}});
