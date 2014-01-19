window.onload = function(){
	
	var sr = $id("search");
	if(sr)
		sr.bind({
			keyup : function(e){
				e = e||window.event;
				var t = e.target||e.srcElement;
				var v = t.value;
				if(v.indexOf(".inc")>0){
					location.href = "?f="+v;
				}
			}
		});
	
	var res = document.getElementsByTagName("code");
	for(var i=0;i<res.length;i++)
		res[i].innerHTML = res[i].innerHTML.toString().replace(/\n/g,"<br>").replace(/<br>/,"").replace(/\t/g,"&nbsp;&nbsp;&nbsp;&nbsp;");
	
	
}