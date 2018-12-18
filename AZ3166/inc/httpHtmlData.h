// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

#define HTTP_STATUS_200 "HTTP/1.0 200 OK"
#define HTTP_STATUS_302 "HTTP/1.1 302 Found" // temporary redirect
#define HTTP_STATUS_404 "HTTP/1.0 404 Not Found"
#define HTTP_STATUS_500 "HTTP/1.0 500 Internal Error"

#define HTTP_HEADER_NO_CACHE "\r\nContent-Type: text/html; \
charset=utf-8\r\nCache-Control: no-cache, no-store, \
must-revalidate\r\n\r\n"

#define HTTP_HEADER_HTML HTTP_HEADER_NO_CACHE "\r\n<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"> <title>Azure IoT Central Device Config</title> <style>@charset \"UTF-8\"; /*Flavor name: Default (mini-default)Author: Angelos Chalaris (chalarangelo@gmail.com)Maintainers: Angelos Chalarismini.css version: v2.1.5 (Fermion)*/ /*Browsers resets and base typography.*/ html{font-size: 16px;}html, *{font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", \"Roboto\", \"Droid Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif; line-height: 1.5; -webkit-text-size-adjust: 100%;}*{font-size: 1rem;}body{margin: 0; color: #212121; background: #f8f8f8;}section{display: block;}input{overflow: visible;}h1, h2{line-height: 1.2em; margin: 0.75rem 0.5rem; font-weight: 500;}h2 small{color: #424242; display: block; margin-top: -0.25rem;}h1{font-size: 2rem;}h2{font-size: 1.6875rem;}p{margin: 0.5rem;}small{font-size: 0.75em;}a{color: #0277bd; text-decoration: underline; opacity: 1; transition: opacity 0.3s;}a:visited{color: #01579b;}a:hover, a:focus{opacity: 0.75;}/*Definitions for the grid system.*/ .container{margin: 0 auto; padding: 0 0.75rem;}.row{box-sizing: border-box; display: -webkit-box; -webkit-box-flex: 0; -webkit-box-orient: horizontal; -webkit-box-direction: normal; display: -webkit-flex; display: flex; -webkit-flex: 0 1 auto; flex: 0 1 auto; -webkit-flex-flow: row wrap; flex-flow: row wrap;}[class^='col-sm-']{box-sizing: border-box; -webkit-box-flex: 0; -webkit-flex: 0 0 auto; flex: 0 0 auto; padding: 0 0.25rem;}.col-sm-10{max-width: 83.33333%; -webkit-flex-basis: 83.33333%; flex-basis: 83.33333%;}.col-sm-offset-1{margin-left: 8.33333%;}@media screen and (min-width: 768px){.col-md-4{max-width: 33.33333%; -webkit-flex-basis: 33.33333%; flex-basis: 33.33333%;}.col-md-offset-4{margin-left: 33.33333%;}}/*Definitions for navigation elements.*/ header{display: block; height: 2.75rem; background: #1e6bb8; color: #f5f5f5; padding: 0.125rem 0.5rem; white-space: nowrap; overflow-x: auto; overflow-y: hidden;}header .logo{color: #f5f5f5; font-size: 1.35rem; line-height: 1.8125em; margin: 0.0625rem 0.375rem 0.0625rem 0.0625rem; transition: opacity 0.3s;}header .logo{text-decoration: none;}/*Definitions for forms and input elements.*/ form{background: #eeeeee; border: 1px solid #c9c9c9; margin: 0.5rem; padding: 0.75rem 0.5rem 1.125rem;}.input-group{display: inline-block;}.input-group.fluid{display: -webkit-box; -webkit-box-pack: justify; display: -webkit-flex; display: flex; -webkit-align-items: center; align-items: center; -webkit-justify-content: center; justify-content: center;}.input-group.fluid>input{-webkit-box-flex: 1; max-width: 100%; -webkit-flex-grow: 1; flex-grow: 1; -webkit-flex-basis: 0; flex-basis: 0;}@media screen and (max-width: 767px){.input-group.fluid{-webkit-box-orient: vertical; -webkit-align-items: stretch; align-items: stretch; -webkit-flex-direction: column; flex-direction: column;}}[type=\"password\"], select{box-sizing: border-box; background: #fafafa; color: #212121; border: 1px solid #c9c9c9; border-radius: 2px; margin: 0.25rem; padding: 0.5rem 0.75rem;}[type=\"text\"], select{box-sizing: border-box; background: #fafafa; color: #212121; border: 1px solid #c9c9c9; border-radius: 2px; margin: 0.25rem; padding: 0.5rem 0.75rem;}fieldset.group{margin: 0; padding: 0; margin-bottom: 0.25em; margin-top: 0.5em; padding-bottom: 1.125em; padding-top: 0.5em; border: 1px solid #696666;}fieldset.group legend{margin: 0; padding: 0; margin-left: 15px; color: #696666; font-size: 1rem;}ul.checkbox{margin: 0; padding: 0; margin-left: 60px; list-style: none;}ul.checkbox li input{margin-right: .25em;}ul.checkbox li{border: 1px transparent solid; display:inline-block; width:12em;}ul.checkbox li label{margin-left: 5px;}input:not([type=\"button\"]):not([type=\"submit\"]):not([type=\"reset\"]):hover, input:not([type=\"button\"]):not([type=\"submit\"]):not([type=\"reset\"]):focus, select:hover, select:focus{border-color: #0288d1; box-shadow: none;}input:not([type=\"button\"]):not([type=\"submit\"]):not([type=\"reset\"]):disabled, select:disabled{cursor: not-allowed; opacity: 0.75;}::-webkit-input-placeholder{opacity: 1; color: #616161;}::-moz-placeholder{opacity: 1; color: #616161;}::-ms-placeholder{opacity: 1; color: #616161;}::placeholder{opacity: 1; color: #616161;}button::-moz-focus-inner, [type=\"submit\"]::-moz-focus-inner{border-style: none; padding: 0;}button, [type=\"submit\"]{-webkit-appearance: button;}button{overflow: visible; text-transform: none;}button, [type=\"submit\"], a.button, .button{display: inline-block; background: rgba(208, 208, 208, 0.75); color: #212121; border: 0; border-radius: 2px; padding: 0.5rem 0.75rem; margin: 0.5rem; text-decoration: none; transition: background 0.3s; cursor: pointer;}button:hover, button:focus, [type=\"submit\"]:hover, [type=\"submit\"]:focus, a.button:hover, a.button:focus, .button:hover, .button:focus{background: #d0d0d0; opacity: 1;}button:disabled, [type=\"submit\"]:disabled, a.button:disabled, .button:disabled{cursor: not-allowed; opacity: 0.75;}/*Custom elements for forms and input elements.*/ button.primary, [type=\"submit\"].primary, .button.primary{background: rgba(30, 107, 184, 0.9); color: #fafafa;}button.primary:hover, button.primary:focus, [type=\"submit\"].primary:hover, [type=\"submit\"].primary:focus, .button.primary:hover, .button.primary:focus{background: #0277bd;}#content{margin-top: 2em;}</style></head>"
#define HTTP_START_PAGE_HTML_ HTTP_HEADER_HTML "<body> <header> <h1 class=\"logo\">Azure IoT Central Device Config</h1> </header>\
<section class=\"container\"> <div id=\"content\" class=\"row\"> <div class=\"col-sm-10 col-sm-offset-1 col-md-4 col-md-offset-4\" \
style=\"text-align:center;\"><form action=\"/PROCESS\" method=\"get\" id=\"frm\">\
<div class=\"input-group fluid\"> <select name=\"SSID\" id=\"SSID\" style=\"width:100%;\" required>{{networks}}</select> </div>\
<div class=\"input-group fluid\"> <input type=\"password\" value=\"\" name=\"PASS\" id=\"password\" title=\"WiFi Password\" placeholder=\"WiFi Password\" style=\"width:100%;\"> </div>\
<div class=\"input-group fluid\"> <input type=\"password\" value=\"\" name=\"PINCO\" id=\"PINCO\" title=\"Device Pin Code\" placeholder=\"Device Pin Code\" style=\"width:100%;\"> </div>\
<div class=\"input-group fluid\"> <input type=\"text\" value=\"\" name=\"SCOPEID\" id=\"SCOPEID\" title=\"Scope Id\" placeholder=\"Scope Id\" style=\"width:100%;\"> </div>\
<div class=\"input-group fluid\"> <input type=\"text\" value=\"\" name=\"REGID\" id=\"REGID\" title=\"Device Id\" placeholder=\"Device Id\" style=\"width:100%;\"> </div>\
<div class=\"input-group fluid\"> <select name=\"AUTH\" id=\"AUTH\" style=\"width:100%;\" \
required><option value=\"S\">SAS Key</option><option value=\"X\">X509 Certificate</option><option value=\"C\">Connection string</option></select></div>\
<div class=\"input-group fluid\"> <span id=\"link\" style=\"display:none;font-size:80%\"> \
Setting the scope id is enough to authenticate with x509 option. \
This sample firmware already has the client cert builtin. Use \
<a href=\"https://github.com/azure/iot-central-firmware/tree/master/tools/dice\">this tool</a> \
to create the matching sample root cert.</span> <input type=\"text\" value=\"\" \
name=\"SASKEY\" id=\"SASKEY\" title=\"Primary/Secondary device key\" \
placeholder=\"Primary/Secondary device key\" style=\"width:100%;\"> </div>\
<div class=\"input-group fluid\" style=\"text-align:left\"> <fieldset class=\"group\"> \
<legend>Select telemetry data to send</legend> <ul class=\"checkbox\"> \
<li><input type=\"checkbox\" name=\"TEMP\" id=\"temp\" checked><label for=\"temp\">\
Temperature</label></li><li><input type=\"checkbox\" name=\"ACCEL\" id=\"accel\" \
checked><label for=\"accel\">Accelerometer</label></li><li><input type=\"checkbox\" \
name=\"HUM\" id=\"hum\" checked><label for=\"hum\">Humidity</label></li><li><input \
type=\"checkbox\" name=\"GYRO\" id=\"gyro\" checked><label for=\"gyro\">Gyroscope</label></li>\
<li><input type=\"checkbox\" name=\"PRES\" id=\"pres\" checked><label for=\"pres\">\
Pressure</label></li><li><input type=\"checkbox\" name=\"MAG\" id=\"mag\" checked>\
<label for=\"mag\">Magnetometer</label></li></ul> </fieldset> </div><div \
class=\"input-group fluid\" style=\"padding-top: 20px;\"> <button type=\"submit\" \
class=\"primary\">Configure Device</button> </div></form> <h5>Click \
<a href=\"javascript:window.location.href=window.location.href\">here</a> to \
refresh the page if you do not see your network</h5> </div></div></section>\
<script>\
var sel = document.getElementById('AUTH');\
var r = document.getElementById('REGID');\
var s = document.getElementById('SASKEY');\
document.getElementById('frm').onsubmit = function(e) {\
  if (sel.selectedIndex == 2) {\
    str = s.value.toLowerCase(); ind = str.indexOf('deviceid');\
    ind2 = str.indexOf(';', ind + 1); if (ind2 == -1) ind2 = str.length;\
    str = s.value.substr(ind + 9, ind2 - (ind + 9));\
    r.value = str;\
  }\
};\
sel.onchange = function(e) {\
  var s = document.getElementById('SASKEY');\
  var cid = document.getElementById('SCOPEID');\
  var r = document.getElementById('REGID');\
  var l = document.getElementById('link');\
  if (sel.selectedIndex == 1) {\
    s.style='display:none'; r.value = 'riot-device-cert'; r.disabled = true;\
    l.style='';\
    cid.style='';\
    r.style='';\
  } \
  else if (sel.selectedIndex == 2) {\
    s.title='Connection String'; \
    s.placeholder='Connection String';\
    cid.style='display:none';\
    r.style='display:none';\
  } \
  else {\
    l.style='display:none';\
    cid.style='';\
    r.style='';\
    s.title = 'Primary/Secondary device key'; \
    s.placeholder = 'Primary/Secondary device key'; \
    s.style = ''; r.value=''; r.disabled = undefined;\
  }\
}\
</script>\
</body></html>"

#define HTTP_START_PAGE_HTML HTTP_STATUS_200 HTTP_START_PAGE_HTML_
#define HTTP_404_RESPONSE HTTP_STATUS_404 HTTP_HEADER_NO_CACHE

#define HTTP_COMPLETE_RESPONSE HTTP_STATUS_200 HTTP_HEADER_HTML \
"<body> <header> <h1 class=\"logo\">Azure IoT Central Config Complete</h1>  \
</header> <section class=\"container\"> <div id=\"content\" class=\"row\">  \
<div class=\"col-sm-10 col-sm-offset-1 col-md-4 col-md-offset-4\" style=\"  \
text-align:center;\"> <h5>Device configured, please press the board's \"Reset\
\" button to start sending data</h5> </div></div></section></body></html>"

// BEWARE! This is sample only. Consider using password / certificate etc.
#define HTTP_MAIN_PAGE_RESPONSE HTTP_STATUS_200 HTTP_HEADER_HTML \
"<body> <header> <h1 class=\"logo\">Azure IoT Central Config Complete</h1>  \
</header> <section class=\"container\"> <div id=\"content\" class=\"row\">  \
<div class=\"col-sm-10 col-sm-offset-1 col-md-4 col-md-offset-4\" style=\"  \
text-align:center;\"> <h5><br/> Click <a href=\"START/\">Here</a> to setup  \
AZ3166</h5> </div></div></section></body></html>"

#define HTTP_ERROR_PAGE_RESPONSE HTTP_STATUS_200 HTTP_HEADER_HTML \
"<body> <header> <h1 class=\"logo\">Azure IoT Central Config Complete</h1>  \
</header> <section class=\"container\"> <div id=\"content\" class=\"row\">  \
<div class=\"col-sm-10 col-sm-offset-1 col-md-4 col-md-offset-4\" style=\"  \
text-align:center;\"> <h5>Something went wrong. Take a look at the serial   \
monitor.</h5> </div></div></section></body></html>"

#define HTTP_REDIRECT_RESPONSE  HTTP_STATUS_302 \
"\r\nLocation: /COMPLETE\r\n\r\n\r\n"

#define HTTP_REDIRECT_WRONG_PINCODE HTTP_STATUS_200 HTTP_HEADER_HTML \
"<body> <header> <h1 class=\"logo\">Azure IoT Central Config Complete</h1>  \
</header> <section class=\"container\"> <div id=\"content\" class=\"row\">  \
<div class=\"col-sm-10 col-sm-offset-1 col-md-4 col-md-offset-4\" style=\"  \
text-align:center;\"> <h5>Wrong Pin Code.</h5> <h5><br/> Click <a href=\"   \
START/\">Here</a> to setup AZ3166</h5></div></div></section></body></html>"
