var TimeoutID;
var Time;
function form2 (v) {return(v<10?'0'+v:v);}
function showtime () {
   sec0=Time%60;
	min0=(Time%3600)/60|0;
   	hour0=Time/3600|0;
    document.getElementById('clock1').innerHTML = 
    form2(hour0)+':'+form2(min0)+':'+form2(sec0);
    Time++;
    window.setTimeout('showtime();',1000);
}
 function inittime (hour,min,sec) { 
  Time=hour*3600+min*60+sec;
  TimeoutID=window.setTimeout('showtime()', 1000);
}

/*function open_url(url,help_en,help_ru,lang){
  parent.ContentFrame.location.href= url;
  if(lang=='ru'){
  	parent.HelpFrame.location.href = help_ru;
  }else{
  	parent.HelpFrame.location.href = help_en;
  }  
}*/

function open_single(url) 
{
	parent.ContentFrame.location.href= url;
}
