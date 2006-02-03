function domTableEnhance() {
  if(document.getElementById && document.getElementsByTagName) {  
    var tableClass='enhanced';
    var colourClass='enhancedtablecolouredrow';
    var hoverClass='enhancedtablerowhover';
    var alltables,bodies,i,j,addclass,trs,c;
    alltables=document.getElementsByTagName('table');

    for (i=0;i<alltables.length;i++) {
      if(alltables[i].className.match(tableClass)) {
        bodies=alltables[i].getElementsByTagName('tbody');

	for (i=0;i<bodies.length;i++) {
	  trs=bodies[i].getElementsByTagName('tr')

	  for (j=0;j<trs.length;j++) {
	    if(trs[j].getElementsByTagName('td').length>0) {
	      addClass=j%2==0?' '+colourClass:'';
	      trs[j].className=trs[j].className+addClass;
	      trs[j].c=hoverClass;
	      
	      trs[j].onmouseover=function() {
		this.className=this.className+' '+this.c;
	      }
	      
	      trs[j].onmouseout=function() {
	        this.className=this.className.replace(''+this.c,'');
	      }
	    }
	  }
	}
      }
    }
  } 
}

function domCheckboxEnhance() {
  var els=document.getElementsByTagName("input");
  for(var i=0; i < els.length; i++) {
    if (els[i].type=='checkbox') { // its a checkbox
      els[i].style.display='none'; // hide the original checkbox control:  
      var img = document.createElement("img"); // create the graphical alternative:
      
      // initial state of graphical checkbox 
      // is the same as the original checkbox:
       
      if (els[i].checked) {
        img.src="images/theme-default_form-checkbox-checked.png";
        img.title="Checked";

      } else {
        img.src="images/theme-default_form-checkbox-unchecked.png";
        img.title="Unchecked";

      } 
      
      img.onclick= toggleCheckbox;
      img.onmouseover= mouseoverCheckbox;
      img.onmouseout= mouseoutCheckbox;
      
      // insert the new, clickable image into the DOM
      // infront of the original checkbox:
      
      els[i].parentNode.insertBefore(img, els[i]);
    }  
  }
}


function toggleCheckbox() {

// graphical checkbox onclick event handler
  
  // toggle the checkbox state:
  
  if (this.title == "Checked") {

    // toggle the image and title:  
    this.src="images/theme-default_form-checkbox-unchecked.png";
    this.title="Unchecked";
    
    // update the hidden real checkbox to match the state of the graphical 
    // version:
    this.nextSibling.checked=false;
    
  } else {
  
    // toggle the image and title:  
    this.src="images/theme-default_form-checkbox-checked.png";
    this.title="Checked";

    // update the hidden real checkbox to match the state of the graphical 
    // version:
    this.nextSibling.checked=true;
  }
}

function mouseoverCheckbox() {
// graphical checkbox onmouseover event handler
  this.src="images/theme-default_form-checkbox-uncheckedhover.png";
}

function mouseoutCheckbox() {
// graphical checkbox onmouseout event handler
  // toggle the checkbox state:
  if (this.title == "Checked") {
    this.src="images/theme-default_form-checkbox-checked.png";
  } else {
    this.src="images/theme-default_form-checkbox-unchecked.png";
  }
}

function loadjs() {
// Loads startup functions
  // domCheckboxEnhance();
  domTableEnhance(); 
}

window.onload=loadjs;




/* Form validation stuff */
function IsEmpty(aTextField) {
  if ((aTextField.value.length==0) || (aTextField.value==null)) {
    return true;
  }
  else { 
    return false; 
  }
} 

function IsEmail(str) {
  var filter=/^([\w-]+(?:\.[\w-]+)*)@((?:[\w-]+\.)*\w[\w-]{0,66})\.([a-z]{2,6}(?:\.[a-z]{2})?)$/i
  if (filter.test(str)) {
    return true
  } else {
    return false
  }
}

function checkServerForm(form) {
  if((IsEmpty(form.servername)) || (form.servername.value == "Add your server here"))  {			  
    alert('You must provide a valid server name') ;
    form.servername.focus(); 
    return false; 
  } 
  
  if((isNaN(parseInt(form.serverport.value))) || (form.serverport.value>65535) || (form.serverport.value<1)){ 
    alert('The "port" field must be a valid number (1-65535).');
    form.serverport.focus(); 
    return false; 
  } 
   return true;
}

function checkFeedbackForm(form) {
  if(IsEmpty(form.name))  {			  
    alert('You must provide your name') ;
    form.name.focus(); 
    return false; 
  } 

  if((IsEmpty(form.email)) || (!IsEmail(form.email.value)))  {			  
    alert('You must provide a valid email address (it will NOT appear in clear spammable form on the site)') ;
    form.email.focus(); 
    return false; 
  } 

  if(IsEmpty(form.text))  {			  
    alert('You forgot to type in your message') ;
    form.text.focus(); 
    return false; 
  } 
   
   return true;
}
