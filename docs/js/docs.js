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
	
}