var browserName = navigator.appName;
var browserVer = parseInt(navigator.appVersion);
var loaded=0;
	
if (browserVer >= 4) {
	loadImg();
}
              
function loadImg () { 
    //PRELOAD MOUSE OVER IMAGES
	home_off = new Image(63,19);
	home_off.src = "shared_graphics/nav/home_off.gif";
	home_on = new Image(63,19);
	home_on.src = "shared_graphics/nav/home_on.gif";

	about_off = new Image(66,19);
	about_off.src = "shared_graphics/nav/about_off.gif";
	about_on = new Image(66,19);
	about_on.src = "shared_graphics/nav/about_on.gif";

	docs_off = new Image(112,19);
	docs_off.src = "shared_graphics/nav/docs_off.gif";
	docs_on = new Image(112,19);
	docs_on.src = "shared_graphics/nav/docs_on.gif";
	
	download_off = new Image(87,19);
	download_off.src = "shared_graphics/nav/download_off.gif";
	download_on = new Image(87,19);
	download_on.src = "shared_graphics/nav/download_on.gif";
	
	news_off = new Image(63,19);
	news_off.src = "shared_graphics/nav/news_off.gif";
	news_on = new Image(63,19);
	news_on.src = "shared_graphics/nav/news_on.gif";
	
	support_off = new Image(80,19);
	support_off.src = "shared_graphics/nav/support_off.gif";
	support_on = new Image(80,19);
	support_on.src = "shared_graphics/nav/support_on.gif";
	
	feedback_off = new Image(84,19);
	feedback_off.src = "shared_graphics/nav/feedback_off.gif";
	feedback_on = new Image(84,19);
	feedback_on.src = "shared_graphics/nav/feedback_on.gif";
	
	thanks_off = new Image(94,19);
	thanks_off.src = "shared_graphics/nav/thanks_off.gif";
	thanks_on = new Image(94,19);
	thanks_on.src = "shared_graphics/nav/thanks_on.gif";

   	loaded = 1;
}

//SWITCH THE IMAGES ON MOUSE OVER   
function switchOff(imgName) {
	if (loaded){
		document.images[imgName].src = eval(imgName + "_off.src"); 
	}
}

function switchOn(imgName) {
	if (loaded) {
		document.images[imgName].src = eval(imgName + "_on.src");
	}
}
