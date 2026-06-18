<?php

declare(strict_types=1);

require_once __DIR__ . '/auth.php';
panel_require_login();

$username = panel_username();
$deviceId = 'karavan-1';

$defaultRelayNames = [
    'İÇ AYDINLATMA', 'DIŞ AYDINLATMA', 'HİDROFOR PMP.', 'TELEVİZYON', 'BUZDOLABI',
    '220V PRİZLER', 'USB/ÇAKMAKLIK', 'EKSTRA YÜK', 'BOŞ 2', 'BOŞ 3',
    'BOŞ 4', 'BOŞ 5', 'BOŞ 6', 'BOŞ 7', 'BOŞ 8', 'BOŞ 9', 'BOŞ 10', 'BOŞ 11', 'BOŞ 12', 'BOŞ 13',
];
?>
<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Ölmezler Doğada Karavan Kontrol</title><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>
<style>@import url('https://fonts.googleapis.com/css2?family=Quicksand:wght@700&display=swap');
body { font-family: 'Quicksand', sans-serif; background-color: #2b2b36; margin: 0; padding: 0; color: #ecf0f1; font-weight: 700; user-select: none; overflow-x: hidden; } *, *:before, *:after { box-sizing: border-box; } @keyframes alertBlink { 0% { box-shadow: inset 0 0 0px transparent, 0 0 0px transparent; border: 1px solid transparent; background-color: #343441; } 50% { box-shadow: inset 0 0 25px rgba(231,76,60,0.3), 0 0 15px rgba(231,76,60,0.8); border: 1px solid #e74c3c; background-color: rgba(231,76,60,0.15); } 100% { box-shadow: inset 0 0 0px transparent, 0 0 0px transparent; border: 1px solid transparent; background-color: #343441; } } .alert-blink { animation: alertBlink 1s infinite; border-radius: 12px; } .main-bg { background-color: #2b2b36; max-width: 1920px; margin: 0 auto; display: flex; flex-direction: column; } .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #3f3f4e; padding-bottom: 8px; margin-bottom: 12px; gap: 10px; } .time-box { text-align: left; line-height: 1.2; } .time-box .time { font-size: 24px; color: #f1c40f; } .time-box .date { font-size: 12px; color: #95a5a6; } .brand-box { text-align: center; } .brand-box h1 { margin: 0; font-size: 22px; letter-spacing: 2px; color: #ffffff; } .brand-box .sub { font-size: 10px; color: #3498db; letter-spacing: 2px; text-transform: uppercase; } .conn-box { display: flex; flex-direction: column; align-items: flex-end; gap: 4px; font-size: 11px; color: #bdc3c7; } .wifi-bars { display: flex; align-items: flex-end; gap: 4px; height: 20px; } .wifi-bars span { display: inline-block; width: 5px; background-color: #4a4a5a; border-radius: 2px; transition: 0.3s; } .wifi-bars span:nth-child(1){height:20%;} .wifi-bars span:nth-child(2){height:40%;} .wifi-bars span:nth-child(3){height:60%;} .wifi-bars span:nth-child(4){height:80%;} .wifi-bars span:nth-child(5){height:100%;} .wifi-bars.s1 span:nth-child(1){background-color:#e74c3c;} .wifi-bars.s2 span:nth-child(1),.wifi-bars.s2 span:nth-child(2){background-color:#e67e22;} .wifi-bars.s3 span:nth-child(1),.wifi-bars.s3 span:nth-child(2),.wifi-bars.s3 span:nth-child(3){background-color:#f1c40f;} .wifi-bars.s4 span:nth-child(1),.wifi-bars.s4 span:nth-child(2),.wifi-bars.s4 span:nth-child(3),.wifi-bars.s4 span:nth-child(4){background-color:#2ecc71;} .wifi-bars.s5 span{background-color:#2ecc71;} .dashboard { display: grid; flex-grow: 1; min-height: 0; } .left-panel, .right-panel { display: flex; flex-direction: column; gap: 12px; } .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; } .sensor-card { background: #343441; padding: 10px; border-radius: 12px; text-align: center; border-bottom: 3px solid #3f3f4e; } .sensor-title { font-size: 12px; color: #95a5a6; margin-bottom: 3px; } .sensor-val { font-size: 18px; color: #f1c40f; } .sensor-val.blue { color: #3498db; } .sensor-val.orange { color: #e67e22; } .control-box { background: #343441; border-radius: 12px; padding: 12px; transition: 0.3s; } .box-title { font-size: 15px; margin-bottom: 10px; color: #ecf0f1; border-bottom: 1px solid #4a4a5a; padding-bottom: 6px; display: flex; justify-content: space-between; align-items: center; gap: 8px; } .gauges-box { display: flex; justify-content: space-around; padding: 5px 0; gap: 10px; } .gauge-wrapper { display: flex; flex-direction: column; align-items: center; } .gauge { width: 75px; height: 75px; border-radius: 50%; background: #1a252f; display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 10px rgba(0,0,0,0.5); transition: background 0.3s; margin-top: 5px; } .gauge-inner { width: 60px; height: 60px; background-color: #343441; border-radius: 50%; display: flex; align-items: center; justify-content: center; box-shadow: inset 0 2px 5px rgba(0,0,0,0.8); } .power-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; } .power-card { background-color: #343441; padding: 8px; border-radius: 12px; cursor: pointer; transition: all 0.3s ease; border: 2px solid transparent; display: flex; flex-direction: column; align-items: center; text-align: center; } .power-card svg { margin-bottom: 3px; transition: 0.3s; width: 28px; height: 28px; } .power-card.on { border-color: #27ae60; box-shadow: 0 0 12px rgba(39,174,96,0.3); } .power-card.on svg { color: #27ae60; filter: drop-shadow(0 0 4px #27ae60); } .power-card.off { border-color: #e74c3c; box-shadow: 0 0 12px rgba(231,76,60,0.3); } .power-card.off svg { color: #e74c3c; filter: drop-shadow(0 0 4px #e74c3c); } .power-title { font-size: 11px; font-weight: bold; color: #ecf0f1; } .power-status { font-size: 11px; font-weight: bold; color: #ffffff; } .relay-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; } .relay-btn { background: #2b2b36; border: 1px solid #3f3f4e; border-radius: 10px; padding: 8px 2px; cursor: pointer; display: flex; flex-direction: column; align-items: center; justify-content: space-between; height: 75px; transition: 0.2s; outline: none; } .relay-btn:active { transform: scale(0.95); } .relay-btn .label { color: #bdc3c7; font-size: 9px; text-transform: uppercase; font-family: 'Quicksand'; font-weight: 700; line-height: 1.1; word-wrap: break-word; overflow: hidden; text-overflow: ellipsis; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; width: 100%; text-align: center; } .relay-btn svg { width: 22px; height: 22px; stroke: #3498db; fill: none; stroke-width: 1.5; stroke-linecap: round; stroke-linejoin: round; margin: auto 0; transition: 0.3s; } .relay-btn .status-line { width: 60%; height: 4px; border-radius: 2px; background: #e74c3c; transition: 0.3s; box-shadow: 0 0 8px rgba(231,76,60,0.6); } .relay-btn.on .status-line { background: #2ecc71; box-shadow: 0 0 10px rgba(46,204,113,0.8); } .relay-btn.on svg { stroke: #2ecc71; filter: drop-shadow(0 0 3px #2ecc71); } .edit-names-btn { background:#34495e; border:none; color:white; padding:5px 8px; border-radius:6px; font-family:'Quicksand'; font-weight:700; cursor:pointer; font-size:11px; } .btn-all { width:100%; padding:10px; font-size:14px; font-family:'Quicksand'; font-weight:700; border:none; border-radius:10px; background:#8e44ad; color:white; cursor:pointer; text-transform:uppercase; letter-spacing:2px; margin-top:8px;} .heater-grid { display: grid; grid-template-columns: 1fr; gap: 10px; } @media (min-width: 600px) { .heater-grid { grid-template-columns: repeat(3, 1fr); } } .heater-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; gap: 5px; } .heater-target { display: flex; align-items: center; gap: 8px; background: #2b2b36; padding: 5px; border-radius: 10px; justify-content: center; } .heater-target span { font-size: 20px; color: #e67e22; width: 35px; text-align: center; } .btn-circle { width: 30px; height: 30px; border-radius: 50%; border: none; background: #3f3f4e; color: white; font-size: 18px; font-weight: 700; cursor: pointer; display: flex; align-items: center; justify-content: center; transition: 0.2s; } .btn-circle:active { transform: scale(0.9); } .btn-action { background:#2980b9; border:none; color:white; padding:6px 10px; border-radius:6px; font-family:'Quicksand'; font-weight:700; cursor:pointer; font-size:11px; } .btn-auto { width:100%; padding:8px; border:none; border-radius:8px; font-family:'Quicksand'; font-weight:700; font-size:12px; margin-top:6px; cursor:pointer; transition:0.3s; color:white;} .disabled-area { opacity:0.3; pointer-events:none; filter:grayscale(100%); transition:0.3s; } .pwr-btn { padding:5px 8px; border-radius:6px; font-family:'Quicksand'; font-weight:700; font-size:11px; color:white; border:none; cursor:pointer; transition:0.3s; } .pwr-on{background:#27ae60;} .pwr-off{background:#e74c3c;} .on-bg{background-color:#27ae60!important;} .off-bg{background-color:#e74c3c!important;} .terazi-rain-grid { display: grid; grid-template-columns: 1fr; gap: 12px; } .terazi-wrap, .rain-wrap { display: flex; justify-content: space-between; align-items: center; background: #1a252f; padding: 12px; border-radius: 12px; box-shadow: inset 0 0 8px rgba(0,0,0,0.5); gap: 10px; height: 82px; } .cv-pitch{transform-origin:60% 75%;transition:transform 0.4s ease-out;} .cv-roll{transform-origin:50% 75%;transition:transform 0.4s ease-out;} .val-text{font-size:18px;color:#f1c40f;font-weight:700;background:#1a252f;padding:5px 12px;border-radius:10px;box-shadow:inset 0 2px 4px rgba(0,0,0,0.5);border:1px solid #3f3f4e;display:inline-block;}
.user-bar{display:flex;align-items:center;gap:8px;font-size:11px;color:#95a5a6;}
.user-bar a{color:#e74c3c;text-decoration:none;padding:4px 8px;border:1px solid #e74c3c;border-radius:6px;}
.online-badge{font-size:10px;padding:3px 8px;border-radius:6px;color:#fff;background:#e74c3c;}
.online-badge.on{background:#27ae60;}
.offline-banner{background:rgba(231,76,60,0.2);border:1px solid #e74c3c;color:#ffb3aa;padding:8px 12px;border-radius:8px;font-size:12px;text-align:center;margin-bottom:10px;display:none;}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#2c3e50;color:#fff;padding:10px 18px;border-radius:8px;font-size:13px;z-index:9999;opacity:0;transition:opacity .3s;pointer-events:none;border:1px solid #4a4a5a;}
.toast.show{opacity:1;}
.panel-busy{opacity:.65;pointer-events:none;}
@media (orientation: landscape) {
  html,body{height:100%;overflow:hidden;}
  .main-bg{height:100vh;padding:10px 15px;}
  .dashboard{grid-template-columns:320px 1fr;gap:12px;height:calc(100vh - 65px);}
  .left-panel{height:100%;overflow-y:auto;padding-right:4px;}
  .right-panel{height:100%;overflow-y:auto;padding-right:4px;display:flex;flex-direction:column;justify-content:space-between;gap:10px;}
  .power-grid,.heater-grid{flex-shrink:0;}
  .right-panel > .control-box{flex-grow:1.5;display:flex;flex-direction:column;margin-bottom:0;padding-bottom:10px;}
  .relay-grid{flex-grow:1;grid-template-columns:repeat(10,1fr);gap:6px;align-items:stretch;}
  .relay-btn{height:100%;min-height:50px;padding:5px 2px;justify-content:center;gap:5px;}
  .relay-btn svg{width:20px;height:20px;margin:0;}
  .relay-btn .label{font-size:8px;}
  .btn-all{margin-top:auto;flex-shrink:0;}
  .terazi-rain-grid{flex-grow:1;grid-template-columns:1fr 1fr;align-items:stretch;display:grid;gap:12px;}
  .terazi-rain-grid .control-box{display:flex;flex-direction:column;justify-content:space-between;height:100%;margin:0;padding-bottom:10px;}
  .terazi-wrap,.rain-wrap{flex-grow:1;height:100%;margin-top:5px;}
  .left-panel::-webkit-scrollbar,.right-panel::-webkit-scrollbar{width:4px;}
  .left-panel::-webkit-scrollbar-thumb,.right-panel::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.15);border-radius:2px;}
}
@media (orientation: portrait) {
  html,body{overflow-y:auto;}
  .main-bg{min-height:100vh;height:auto;padding:12px;}
  .relay-grid{grid-template-columns:repeat(4,1fr);gap:6px;}
  .relay-btn{height:75px;}
  .relay-btn .label{font-size:9px;}
  .terazi-wrap,.rain-wrap{flex-direction:row;justify-content:space-around;}
  .left-panel,.right-panel{display:contents;}
  .dashboard{display:flex;flex-direction:column;gap:15px;}
  .network-box{order:1;} .sensor-grid{order:2;} .energy-box{order:3;} .power-grid-wrap{order:4;margin-top:0;}
  .relay-box{order:5;} .heater-grid{order:6;display:flex!important;flex-direction:column!important;gap:15px;}
  .heater-boiler{order:1;} .heater-webasto{order:2;} .heater-fridge{order:3;}
  .terazi-rain-grid{order:7;display:flex!important;flex-direction:column!important;gap:15px;}
  .terazi-box{order:1;} .rain-box{order:2;} .gas-alarm-box{order:8;margin-top:0;}
}
</style></head><body><div class='main-bg'>

<div id='offline_banner' class='offline-banner'>Karavan çevrimdışı — son veri bekleniyor veya cihaz internete bağlı değil.</div>

<div class='header'>
  <div class='time-box'><div id='sys_time' class='time'>--:--</div><div id='sys_date' class='date'>--.--.---- <span id='sys_day'></span></div></div>
  <div class='brand-box'><h1>ÖLMEZLER DOĞADA</h1><div class='sub'>KARAVAN KONTROL SİSTEMİ</div></div>
  <div style='display:flex;align-items:center;gap:15px;'>
    <div class='user-bar'>
      <span id='online_badge' class='online-badge'>OFFLINE</span>
      <span><?= htmlspecialchars($username, ENT_QUOTES, 'UTF-8') ?></span>
      <a href='/panel/logout.php'>Çıkış</a>
    </div>
    <button id='mic_btn' onclick='startVoiceControl()' style='background:#e74c3c;border:none;border-radius:50%;width:45px;height:45px;cursor:pointer;box-shadow:0 0 10px rgba(231,76,60,0.6);display:flex;justify-content:center;align-items:center;transition:0.3s;'>
      <svg viewBox='0 0 24 24' width='22' height='22' stroke='white' stroke-width='2' fill='none' stroke-linecap='round' stroke-linejoin='round'><path d='M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z'></path><path d='M19 10v2a7 7 0 0 1-14 0v-2'></path><line x1='12' y1='19' x2='12' y2='23'></line><line x1='8' y1='23' x2='16' y2='23'></line></svg>
    </button>
    <div class='conn-box'><div id='wifi_bars' class='wifi-bars s0'><span></span><span></span><span></span><span></span><span></span></div><div id='wifi_text'>Sinyal Bekleniyor</div></div>
  </div>
</div>

<div class='dashboard'>
  <div class='left-panel'>
    <div class='control-box network-box'>
      <div class='box-title'>Sistem Ağ Durumu</div>
      <div style='display:flex;justify-content:space-around;align-items:center;gap:10px;'>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 2<br><span id='web_rs2' style='font-size:15px;'>-- dBm</span></div>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 3<br><span id='web_rs3' style='font-size:15px;'>-- dBm</span></div>
        <div style='text-align:center;color:#95a5a6;font-size:12px;'>Kart 4<br><span id='web_rs4' style='font-size:15px;'>-- dBm</span></div>
      </div>
    </div>
    <div class='sensor-grid'>
      <div class='sensor-card'><div class='sensor-title'>Gün Doğumu</div><div id='web_sunrise' class='sensor-val orange'>--:--</div></div>
      <div class='sensor-card'><div class='sensor-title'>Gün Batımı</div><div id='web_sunset' class='sensor-val orange'>--:--</div></div>
      <div class='sensor-card'><div class='sensor-title'>İç Ortam Sıcaklığı</div><div id='web_temp' class='sensor-val'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Dış Ortam Sıcaklığı</div><div id='web_t_out' class='sensor-val blue'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Su Sıcaklığı</div><div id='web_t_wat' class='sensor-val blue'>-- °C</div></div>
      <div class='sensor-card'><div class='sensor-title'>Konum</div><div id='web_location' class='sensor-val' style='font-size:14px;word-break:break-all;'>Yükleniyor...</div></div>
    </div>
    <div class='control-box energy-box'>
      <div class='box-title'>Depo ve Enerji Durumu</div>
      <div class='gauges-box'>
        <div class='gauge-wrapper'><div class='sensor-title'>Temiz Su Miktarı</div><div class='gauge' id='water_gauge'><div class='gauge-inner'><span id='web_water_p' style='color:#3498db;font-size:16px;'>-- %</span></div></div><div id='web_water_l' style='margin-top:5px;color:#bdc3c7;font-size:11px;'>-- L</div></div>
        <div class='gauge-wrapper'><div class='sensor-title'>LiFePO4 Voltajı</div><div class='gauge' id='bat_gauge'><div class='gauge-inner'><span id='web_bat_p' style='font-size:16px;'>-- %</span></div></div><div id='web_bat_v' style='margin-top:5px;color:#bdc3c7;font-size:11px;'>-- V</div></div>
      </div>
    </div>
    <div id='box_gas_alarm' class='control-box gas-alarm-box' onclick='toggleGasAlarm()' style='cursor:pointer;'>
      <div class='box-title'><span>Gaz Güvenlik Sistemi</span><span id='web_gas_al' style='font-size:10px;padding:3px 8px;border-radius:6px;color:#fff;'>YÜKLENİYOR...</span></div>
      <div style='display:grid;grid-template-columns:1fr;gap:10px;text-align:center;'>
        <div style='background:#2b2b36;padding:10px;border-radius:8px;'><div class='sensor-title'>LPG / Duman</div><div id='web_mq2' style='font-size:20px;font-weight:bold;'>--</div></div>
      </div>
    </div>
  </div>

  <div class='right-panel'>
    <div class='power-grid power-grid-wrap'>
      <div id='card_bat_main' class='power-card off' onclick='toggleMainBat()'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='2' y='7' width='16' height='10' rx='2' ry='2'></rect><line x1='22' y1='11' x2='22' y2='13'></line><line x1='6' y1='12' x2='6.01' y2='12'></line></svg><div class='power-title'>ANA AKÜ (ŞALTER)</div><div id='text_bat_main' class='power-status'>KAPALI</div></div>
      <div id='card_ups' class='power-card off' onclick='toggleUPS()'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='3' y='4' width='18' height='16' rx='2' ry='2'></rect><path d='M8 12 Q 10 8 12 12 T 16 12'></path></svg><div class='power-title'>SMART INVERTER</div><div id='text_ups' class='power-status'>KAPALI</div></div>
    </div>
    <div class='control-box relay-box'>
      <div class='box-title'><span>Karavan Kontrol Ünitesi</span><button class='edit-names-btn' onclick='editRelayNames()'>Düzenle</button></div>
      <div class='relay-grid'>
<?php for ($i = 1; $i <= 20; $i++): $name = htmlspecialchars($defaultRelayNames[$i - 1], ENT_QUOTES, 'UTF-8'); ?>
        <button id='card_r<?= $i ?>' class='relay-btn off' onclick='sendToggle(<?= $i ?>)'>
          <div id='label_r<?= $i ?>' class='label'><?= $name ?></div>
          <svg viewBox='0 0 24 24'><rect x='2' y='7' width='20' height='15' rx='2' ry='2'/></svg>
          <div class='status-line'></div>
        </button>
<?php endfor; ?>
      </div>
      <button class='btn-all' onclick='sendAllOff()'>TÜMÜNÜ KAPAT</button>
    </div>
    <div class='terazi-rain-grid'>
      <div class='control-box terazi-box'>
        <div class='box-title'>Akıllı Karavan Terazisi</div>
        <div class='terazi-wrap'>
          <div style='text-align:center;'><div class='sensor-title' style='margin-bottom:3px;color:#bdc3c7;font-size:11px;'>Ön - Arka</div><div id='web_pitch' class='val-text'>-- °</div></div>
          <div style='display:flex;gap:12px;align-items:flex-end;justify-content:center;'>
            <svg class='cv-pitch' id='cv_pitch' viewBox='0 0 100 60' width='75' height='45'><line x1='5' y1='45' x2='25' y2='45' stroke='#95a5a6' stroke-width='4'/><path d='M 25 20 Q 30 5 45 5 L 85 5 Q 95 5 95 15 L 95 45 L 25 45 Z' fill='#ecf0f1'/><rect x='35' y='15' width='20' height='12' rx='2' fill='#3498db'/><rect x='65' y='15' width='15' height='25' rx='2' fill='#bdc3c7'/><circle cx='60' cy='45' r='10' fill='#2c3e50'/><circle cx='60' cy='45' r='4' fill='#bdc3c7'/></svg>
            <svg class='cv-roll' id='cv_roll' viewBox='0 0 60 60' width='45' height='45'><path d='M 10 45 L 10 15 Q 10 5 20 5 L 40 5 Q 50 5 50 15 L 50 45 Z' fill='#ecf0f1'/><rect x='15' y='15' width='30' height='12' rx='2' fill='#3498db'/><circle cx='10' cy='45' r='8' fill='#2c3e50'/><circle cx='50' cy='45' r='8' fill='#2c3e50'/></svg>
          </div>
          <div style='text-align:center;'><div class='sensor-title' style='margin-bottom:5px;color:#bdc3c7;font-size:11px;'>Sağ - Sol</div><div id='web_roll' class='val-text'>-- °</div></div>
        </div>
      </div>
      <div id='box_rain_alarm' class='control-box rain-box' onclick='toggleRainAlarm()' style='cursor:pointer;'>
        <div class='box-title'><span>Hava ve Yağmur Durumu</span><span id='web_rain_al' style='font-size:10px;padding:3px 8px;border-radius:6px;color:#fff;'>YÜKLENİYOR...</span></div>
        <div class='rain-wrap'>
          <div style='display:flex;align-items:center;gap:10px;'>
            <svg viewBox='0 0 24 24' width='32' height='32' fill='none' stroke='#3498db' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M20 17.58A5 5 0 0 0 18 8h-1.26A8 8 0 1 0 4 16.25'/><line x1='8' y1='16' x2='6' y2='21'/><line x1='12' y1='17' x2='10' y2='22'/><line x1='16' y1='16' x2='14' y2='21'/></svg>
            <div><div id='web_rain_status' style='font-size:15px;color:#2ecc71;'>Hava Temiz</div><div class='sensor-title' style='font-size:11px;margin:0;'>Yağmur Yoğunluğu</div></div>
          </div>
          <div id='web_rain' class='val-text'>-- %</div>
        </div>
      </div>
    </div>
    <div class='heater-grid'>
      <div class='control-box heater-boiler' style='margin:0;'>
        <div class='box-title'><span>Su Isıtıcı Rezistans</span></div>
        <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_wh_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualWaterHeater()'>Tetikle</button></div>
        <div class='heater-target'><button class='btn-circle' onclick='changeWaterTarget(-1)'>-</button><span id='web_wh_target' style='color:#3498db;'>--</span><button class='btn-circle' onclick='changeWaterTarget(1)'>+</button></div>
        <button id='btn_wh_auto' class='btn-auto' onclick='toggleWaterAutoMode()'>Otomatik Mod: --</button>
      </div>
      <div class='control-box heater-webasto' style='margin:0;'>
        <div class='box-title'><span>Webasto Isıtıcı</span><button id='btn_w_pwr' class='pwr-btn pwr-off' onclick='toggleWebastoPower()'>GÜÇ: KAPALI</button></div>
        <div id='webasto_controls' class='disabled-area'>
          <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_w_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualWebasto()'>Tetikle</button></div>
          <div id='web_w_info' style='font-size:11px;color:#95a5a6;margin-bottom:10px;text-align:center;'>--</div>
          <div class='heater-target'><button class='btn-circle' onclick='changeTarget(-1)'>-</button><span id='web_w_target' style='color:#e67e22;'>--</span><button class='btn-circle' onclick='changeTarget(1)'>+</button></div>
          <button id='btn_w_auto' class='btn-auto' onclick='toggleAutoMode()'>Otomatik Mod: --</button>
        </div>
      </div>
      <div class='control-box heater-fridge' style='margin:0;'>
        <div class='box-title'><span>Buzdolabı Fanı</span><span id='web_t_ext_fridge' style='color:#f1c40f;font-size:14px;font-weight:bold;'>-- °C</span></div>
        <div class='heater-row'><span style='color:#bdc3c7;'>Durum:</span><span id='web_f_status' style='font-size:14px;'>--</span><button class='btn-action' onclick='manualFridge()'>Tetikle</button></div>
        <div class='heater-target'><button class='btn-circle' onclick='changeFridgeTarget(-1)'>-</button><span id='web_f_target' style='color:#3498db;'>--</span><button class='btn-circle' onclick='changeFridgeTarget(1)'>+</button></div>
        <button id='btn_f_auto' class='btn-auto' onclick='toggleFridgeAuto()'>Otomatik Mod: --</button>
      </div>
    </div>
  </div>
</div>
</div>
<div id='toast' class='toast'></div>

<script>
const PANEL_API = '/panel/api.php';
const DEVICE_ID = <?= json_encode($deviceId, JSON_UNESCAPED_UNICODE) ?>;

window.lastData = {};
window.deviceOnline = false;
let rainWarned = false;
let smokeWarned = false;
let commandBusy = false;

function speak(text) { if('speechSynthesis' in window){let m=new SpeechSynthesisUtterance(text);m.lang='tr-TR';window.speechSynthesis.speak(m);} }

function showToast(msg, isError = false) {
  const t = document.getElementById('toast');
  if (!t) return;
  t.textContent = msg;
  t.style.background = isError ? '#c0392b' : '#2c3e50';
  t.classList.add('show');
  clearTimeout(window._toastTimer);
  window._toastTimer = setTimeout(() => t.classList.remove('show'), 2500);
}

function setPanelBusy(busy) {
  commandBusy = busy;
  const root = document.querySelector('.main-bg');
  if (root) root.classList.toggle('panel-busy', busy);
}

function apiCommand(action, params = {}) {
  if (commandBusy) return Promise.resolve({ success: false, message: 'Bekleyin...' });
  if (!window.deviceOnline) showToast('Cihaz çevrimdışı — komut kuyruğa alınacak', true);

  setPanelBusy(true);
  return fetch(PANEL_API + '?device_id=' + encodeURIComponent(DEVICE_ID), {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action, params, device_id: DEVICE_ID })
  })
  .then(async r => {
    const data = await r.json().catch(() => ({}));
    if (r.status === 401) { window.location.href = '/panel/login.php'; return { success: false }; }
    if (!r.ok || !data.success) {
      showToast(data.message || 'Komut gönderilemedi', true);
      return data;
    }
    showToast('Komut gönderildi');
    return data;
  })
  .catch(() => {
    showToast('Bağlantı hatası', true);
    return { success: false };
  })
  .finally(() => setPanelBusy(false));
}

function afterCommand() {
  [800, 2000, 4000, 8000, 12000].forEach(ms => setTimeout(fetchStatus, ms));
}

function fetchSunTimes() {
  fetch('https://get.geojs.io/v1/ip/geo.json').then(r=>r.json()).then(d=>{
    let locEl = document.getElementById('web_location');
    if(locEl) {
      let city = d.city ? d.city : "";
      let region = d.region ? d.region.replace(" Province", "") : "";
      let country = d.country ? d.country : "";
      if (city !== "" && region !== "" && city !== region) locEl.innerHTML = city + " / " + region;
      else if (region !== "") locEl.innerHTML = region;
      else if (city !== "") locEl.innerHTML = city;
      else if (country !== "") locEl.innerHTML = country;
      else locEl.innerHTML = "Konum Bulunamadı";
    }
    return fetch('https://api.sunrise-sunset.org/json?lat='+d.latitude+'&lng='+d.longitude+'&formatted=0');
  }).then(r=>r.json()).then(d=>{
    let ss=new Date(d.results.sunset).toLocaleTimeString('tr-TR',{hour:'2-digit',minute:'2-digit'});
    let sr=new Date(d.results.sunrise).toLocaleTimeString('tr-TR',{hour:'2-digit',minute:'2-digit'});
    let ssEl=document.getElementById('web_sunset');  if(ssEl)ssEl.innerHTML=ss;
    let srEl=document.getElementById('web_sunrise'); if(srEl)srEl.innerHTML=sr;
  }).catch(e=>console.log(e));
}
fetchSunTimes(); setInterval(fetchSunTimes,3600000);

function set(id,val){let e=document.getElementById(id);if(e)e.innerHTML=val;}
function setStyle(id,prop,val){let e=document.getElementById(id);if(e)e.style[prop]=val;}
function setClass(id,cls){let e=document.getElementById(id);if(e)e.className=cls;}

function num(v, def=0){ return (v === undefined || v === null || isNaN(v)) ? def : Number(v); }

function setOnlineBadge(online) {
  window.deviceOnline = !!online;
  const el = document.getElementById('online_badge');
  const banner = document.getElementById('offline_banner');
  if (el) { el.textContent = online ? 'ONLINE' : 'OFFLINE'; el.className = 'online-badge' + (online ? ' on' : ''); }
  if (banner) banner.style.display = online ? 'none' : 'block';
}

function fetchStatus() {
  Promise.all([
    fetch(PANEL_API + '?action=status&device_id=' + encodeURIComponent(DEVICE_ID)).then(r => r.json()),
    fetch(PANEL_API + '?action=online&device_id=' + encodeURIComponent(DEVICE_ID)).then(r => r.json()).catch(() => null)
  ]).then(([res, onlineRes]) => {
    if (onlineRes && onlineRes.success) setOnlineBadge(!!onlineRes.data.online);
    else if (res.success && res.data && res.data.received_at) {
      const age = Date.now() - new Date(res.data.received_at).getTime();
      setOnlineBadge(age < 120000);
    } else setOnlineBadge(false);

    if (!res.success) return;
    const wrap = res.data || {};
    const d = wrap.data || {};
    if (!d || Object.keys(d).length === 0) return;

    window.lastData = d;
    set('sys_time', d.time || '--:--');
    set('sys_date', (d.date || '--.--.----') + ' ' + (d.day || ''));

    let rssi = num(d.rssi, -100);
    let bars = 0;
    if(rssi>-60)bars=5; else if(rssi>-70)bars=4; else if(rssi>-80)bars=3; else if(rssi>-90)bars=2; else if(rssi<0)bars=1;
    setClass('wifi_bars','wifi-bars s'+bars);
    set('wifi_text',bars>0?rssi+' dBm':'Bağlantı Yok');

    setClass('card_bat_main',d.bat_main==1?'power-card on':'power-card off');
    set('text_bat_main',d.bat_main==1?'AÇIK':'KAPALI');
    setClass('card_ups',d.ups==1?'power-card on':'power-card off');
    set('text_ups',d.ups==1?'AÇIK':'KAPALI');

    let wPwr=d.w_pwr;
    let btnPwr=document.getElementById('btn_w_pwr');
    let wCtrl=document.getElementById('webasto_controls');
    if(btnPwr&&wCtrl){if(wPwr==1){btnPwr.className='pwr-btn pwr-on';btnPwr.innerHTML='GÜÇ: AÇIK';wCtrl.className='';}else{btnPwr.className='pwr-btn pwr-off';btnPwr.innerHTML='GÜÇ: KAPALI';wCtrl.className='disabled-area';}}

    for(let i=1;i<=20;i++){
      let c=document.getElementById('card_r'+i);
      if(c)c.className=(d['r'+i]==1)?'relay-btn on':'relay-btn off';
      let l=document.getElementById('label_r'+i); if(l&&d['n'+i])l.innerHTML=d['n'+i];
    }

    set('web_temp',  num(d.temp).toFixed(1)+' °C');
    set('web_t_out', num(d.t_out).toFixed(1)+' °C');
    set('web_t_wat', num(d.t_wat).toFixed(1)+' °C');
    set('web_t_ext_fridge', num(d.t_ext).toFixed(1)+' °C');

    set('web_wh_target', num(d.wh_target).toFixed(0));
    let whSt=document.getElementById('web_wh_status');
    if(whSt){whSt.innerHTML=(d.wh_on==1)?'AÇIK (Isıtıyor)':'KAPALI';whSt.style.color=(d.wh_on==1)?'#27ae60':'#e74c3c';}
    let btnWhAuto=document.getElementById('btn_wh_auto');
    if(btnWhAuto){btnWhAuto.className=(d.wh_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnWhAuto.innerHTML='Otomatik Mod: '+((d.wh_auto==1)?'AÇIK':'KAPALI');}

    set('web_f_target', num(d.f_target).toFixed(0));
    let fSt=document.getElementById('web_f_status');
    if(fSt){fSt.innerHTML=(d.f_on==1)?'AÇIK (Soğutuyor)':'KAPALI';fSt.style.color=(d.f_on==1)?'#3498db':'#e74c3c';}
    let btnFAuto=document.getElementById('btn_f_auto');
    if(btnFAuto){btnFAuto.className=(d.f_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnFAuto.innerHTML='Otomatik Mod: '+((d.f_auto==1)?'AÇIK':'KAPALI');}

    set('web_w_target', num(d.w_target).toFixed(0));
    let wSt=document.getElementById('web_w_status');
    if(wSt){wSt.innerHTML=(d.w_on==1)?'AÇIK':'KAPALI';wSt.style.color=(d.w_on==1)?'#27ae60':'#e74c3c';}
    set('web_w_info', d.w_info || '--');
    let btnWAuto=document.getElementById('btn_w_auto');
    if(btnWAuto){btnWAuto.className=(d.w_auto==1)?'btn-auto on-bg':'btn-auto off-bg';btnWAuto.innerHTML='Otomatik Mod: '+((d.w_auto==1)?'AÇIK':'KAPALI');}

    let wP=num(d.water);
    if(wP<0)wP=0; if(wP>100)wP=100;
    let wColor='hsl('+Math.floor(wP*1.2)+',85%,55%)';
    let wPEl=document.getElementById('web_water_p');
    if(wPEl){wPEl.innerHTML=wP+' %';wPEl.style.color=wColor;}
    set('web_water_l', num(d.water_liter)+' L');
    let wG=document.getElementById('water_gauge'); if(wG)wG.style.background='conic-gradient('+wColor+' '+wP+'%, #1a252f 0)';

    let batV=num(d.bat);
    let batP=num(d.bat_percent);
    let bColor='hsl('+Math.floor(batP*1.2)+',85%,55%)';
    let bPEl=document.getElementById('web_bat_p');
    if(bPEl){bPEl.innerHTML=batP+' %';bPEl.style.color=bColor;}
    set('web_bat_v', batV.toFixed(1)+' V');
    let bG=document.getElementById('bat_gauge'); if(bG)bG.style.background='conic-gradient('+bColor+' '+batP+'%, #1a252f 0)';

    let rs2El=document.getElementById('web_rs2');
    if(rs2El){rs2El.innerHTML=d.rs2<=-100?'Sinyal Yok':d.rs2+' dBm';rs2El.style.color=d.rs2<=-100?'#e74c3c':'#3498db';}
    let rs3El=document.getElementById('web_rs3');
    if(rs3El){rs3El.innerHTML=d.rs3<=-100?'Sinyal Yok':d.rs3+' dBm';rs3El.style.color=d.rs3<=-100?'#e74c3c':'#e67e22';}
    let rs4El=document.getElementById('web_rs4');
    if(rs4El){rs4El.innerHTML=d.rs4<=-100?'Sinyal Yok':d.rs4+' dBm';rs4El.style.color=d.rs4<=-100?'#e74c3c':'#2ecc71';}

    let pitchEl=document.getElementById('web_pitch');
    if(pitchEl){pitchEl.innerHTML=num(d.pitch).toFixed(1)+' °';document.getElementById('cv_pitch').style.transform='rotate('+num(d.pitch)+'deg)';}
    let rollEl=document.getElementById('web_roll');
    if(rollEl){rollEl.innerHTML=num(d.roll).toFixed(1)+' °';document.getElementById('cv_roll').style.transform='rotate('+num(d.roll)+'deg)';}

    let mq2El=document.getElementById('web_mq2');
    if(mq2El){mq2El.innerHTML=d.mq2;mq2El.style.color=d.mq2>1200?'#e74c3c':'#2ecc71';}
    let gasBox=document.getElementById('box_gas_alarm');
    if(gasBox){if(d.mq2>1200)gasBox.classList.add('alert-blink');else gasBox.classList.remove('alert-blink');}

    let rainEl=document.getElementById('web_rain');
    if(rainEl){
      rainEl.innerHTML=d.rain+' %';
      let rStatus='Hava Temiz';
      if(d.rain>65)rStatus='Sağanak Yağmur'; else if(d.rain>20)rStatus='Yağmur Yağıyor'; else if(d.rain>5)rStatus='Hafif Çiseleme';
      let rStEl=document.getElementById('web_rain_status');
      if(rStEl){rStEl.innerHTML=rStatus;rStEl.style.color=d.rain>5?'#3498db':'#2ecc71';}
    }
    let rainBox=document.getElementById('box_rain_alarm');
    if(rainBox){if(d.rain>20)rainBox.classList.add('alert-blink');else rainBox.classList.remove('alert-blink');}

    let gasAlEl=document.getElementById('web_gas_al');
    if(gasAlEl){gasAlEl.innerHTML=d.gas_al?'🔔 SİREN AÇIK':'🔕 SİREN KAPALI';gasAlEl.style.background=d.gas_al?'#27ae60':'#e74c3c';}
    let rainAlEl=document.getElementById('web_rain_al');
    if(rainAlEl){rainAlEl.innerHTML=d.rain_al?'🔔 SİREN AÇIK':'🔕 SİREN KAPALI';rainAlEl.style.background=d.rain_al?'#27ae60':'#e74c3c';}

    if(d.rain>20&&!rainWarned){speak("Dikkat, yağmur başladı. Dışardaki eşyalarınız ıslanmasın.");rainWarned=true;} else if(d.rain<=5){rainWarned=false;}
    if(d.mq2>1200&&!smokeWarned){speak("Tehlike! Karavanda duman veya gaz algılandı.");smokeWarned=true;} else if(d.mq2<=1200){smokeWarned=false;}
  }).catch(e=>{ console.log(e); setOnlineBadge(false); });
}
setInterval(fetchStatus, 2000);
fetchStatus();

function toggleMainBat()   { apiCommand('toggleMainBat').then(afterCommand); }
function toggleUPS()        { apiCommand('toggleUPS').then(afterCommand); }
function sendToggle(id)     { apiCommand('toggle', { id }).then(afterCommand); }
function sendAllOff()       { apiCommand('allOff').then(afterCommand); }
function changeTarget(v)    { apiCommand('setTarget', { val: v }).then(afterCommand); }
function changeWaterTarget(v){ apiCommand('setWaterTarget', { val: v }).then(afterCommand); }
function changeFridgeTarget(v){ apiCommand('setFridgeTarget', { val: v }).then(afterCommand); }
function toggleAutoMode()   { apiCommand('toggleAuto').then(afterCommand); }
function toggleWaterAutoMode(){ apiCommand('toggleWaterAuto').then(afterCommand); }
function toggleFridgeAuto() { apiCommand('toggleFridgeAuto').then(afterCommand); }
function manualWebasto()    { apiCommand('manualWebasto').then(afterCommand); }
function manualWaterHeater(){ apiCommand('manualWaterHeater').then(afterCommand); }
function manualFridge()     { apiCommand('manualFridge').then(afterCommand); }
function toggleWebastoPower(){ apiCommand('toggleWebastoPower').then(afterCommand); }
function toggleGasAlarm()   { apiCommand('toggleGasAlarm').then(afterCommand); }
function toggleRainAlarm()  { apiCommand('toggleRainAlarm').then(afterCommand); }
function editRelayNames() {
  let id = prompt('Düzenlenecek röle numarası (1-20):', '1');
  if (id === null) return;
  id = parseInt(id, 10);
  if (isNaN(id) || id < 1 || id > 20) { alert('Geçersiz röle numarası'); return; }
  let c = document.getElementById('label_r' + id)?.innerText || '';
  let n = prompt('Röle ' + id + ' ismi:', c);
  if (n !== null && n.trim() !== '') apiCommand('setRelayName', { id, name: n.trim() }).then(afterCommand);
}

const SpeechRecognition=window.SpeechRecognition||window.webkitSpeechRecognition;
if(SpeechRecognition){
  const rec=new SpeechRecognition();rec.lang='tr-TR';rec.continuous=false;rec.interimResults=false;
  rec.onstart=function(){document.getElementById('mic_btn').style.background='#2ecc71';document.getElementById('mic_btn').style.boxShadow='0 0 15px rgba(46,204,113,0.8)';};
  rec.onend=function(){document.getElementById('mic_btn').style.background='#e74c3c';document.getElementById('mic_btn').style.boxShadow='0 0 10px rgba(231,76,60,0.6)';};
  rec.onresult=function(e){
    let cmd=e.results[0][0].transcript.toLowerCase();
    if(cmd.includes('iç ortam sıcaklığı')){speak("İç ortam sıcaklığı "+num(window.lastData.temp).toFixed(1)+" derece.");}
    else if(cmd.includes('dış ortam sıcaklığı')){speak("Dış ortam sıcaklığı "+num(window.lastData.t_out).toFixed(1)+" derece.");}
    else if(cmd.includes('su sıcaklığı')){speak("Su sıcaklığı "+num(window.lastData.t_wat).toFixed(1)+" derece.");}
    else if(cmd.includes('temiz su')){speak("Temiz su miktarınız yüzde "+window.lastData.water+".");}
    else if(cmd.includes('akü voltajı')){speak("Lifepo4 akü voltajı "+num(window.lastData.bat).toFixed(1)+" volt.");}
    else if(cmd.includes('durum raporu')||cmd.includes('rapor ver')){speak("Sırasıyla söylüyorum Hüseyin. İç ortam sıcaklığı "+num(window.lastData.temp).toFixed(1)+" derece, dış ortam sıcaklığı "+num(window.lastData.t_out).toFixed(1)+" derece, temiz su miktarı yüzde "+window.lastData.water+", akü voltajı "+num(window.lastData.bat).toFixed(1)+" volt, yağmur durumu yüzde "+window.lastData.rain+".");}
    else if(cmd.includes('iç aydınlatma')||cmd.includes('iç ışıkları')){sendToggle(1);speak("İç aydınlatma durumu değiştirildi.");}
    else if(cmd.includes('dış aydınlatma')||cmd.includes('dış ışıkları')){sendToggle(2);speak("Dış aydınlatma durumu değiştirildi.");}
    else if(cmd.includes('hidrofor')||cmd.includes('su pompası')){sendToggle(3);speak("Hidrofor tetiklendi.");}
    else speak("Lütfen tekrar edin.");
  };
  window.startVoiceControl=function(){rec.start();};
}else{window.startVoiceControl=function(){alert('Tarayıcınız ses tanımayı desteklemiyor. Lütfen Chrome veya Safari kullanın.');};}
</script></body></html>
