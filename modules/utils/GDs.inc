<?php
/**
 *	@file: GDs.inc
 *	@author: Soyoes 2011/07/14
 *	@uses: Combile, resize, drawText on image.
 *	@example:
 *****************************************************************************/

class GDs{

	static $import_dir;
	static $export_dir;
	static $font_dir;
	/**
	 * php_gd2
	 * @param out : relative export path under OUT_PATH
	 * @param bg : background img's relative path
	 * @param target : target/forground img file 's relative path
	 * @param x, y target's coordinations upon background image
	 * @param w, h target's new size
	 *
	 * @example: self::merge('out.gif', 'bg.gif', 'rune/target.gif', 30, 20, 80,60); //merge out.gif to bg.gif at (30,20) of bg.gif , and enlarge/shrink target.gif to 80*60
	 * @example: self::merge('out1.gif', 'bg.jpg', 'item1.gif', 100, 20);
	 * */
	public static function image($out, $bg, $target, $x, $y, $w=null, $h=null){
		self::preload();
		try{
			$bg = str_starts($bg, "/")? $bg:self::$import_dir.$bg;
			$target = str_starts($target, "/")? $target:self::$import_dir.$target;
			$out = str_starts($out, "/")? $out:self::$export_dir.$out;
			$imgs = array();
			foreach (array($bg, $target) as $filename){
				$parts = explode(".",$filename);
				$ext = $parts[count($parts)-1];
				if($ext == "jpg") $ext = "jpeg";
				$func_name = "imagecreatefrom".$ext;
				$imgs[] = call_user_func($func_name, $filename);
			}
			list($width, $height) = getimagesize($target);
			if($w==null) $w=$width;
			if($h==null) $h=$height;
			imagecopyresized($imgs[0], $imgs[1], $x, $y, 0, 0, $w, $h, $width, $height);
			$parts = explode(".",$out);
			$ext = $parts[count($parts)-1];
			if($ext == "jpg") $ext = "jpeg";
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $imgs[0], $out);
			imagedestroy($imgs[0]);
			imagedestroy($imgs[1]);
		}catch (Exception $e){
			echo $e->getMessage();
		}
	}

	/**
	 * draw text on pic
	 * @$fontname : font name under self::$font_dir without ".ttf". for instance "arial"
	 * @$size	: font size of int value
	 * */
	public static function text($out, $bg, $text, $fontname, $x, $y, $size, $color="#000000"){
		self::preload();
		try{
			//putenv('GDFONTPATH=' . realpath('.'));
			$bg = str_starts($bg, "/")? $bg:self::$import_dir.$bg;
			$out = str_starts($out, "/")? $out:self::$export_dir.$out;
			
			$parts = explode(".",$bg);
			$ext = $parts[count($parts)-1];
			if($ext == "jpg") $ext = "jpeg";
			$func_name = "imagecreatefrom".$ext;
			$img = call_user_func($func_name, $bg);
			$r = hexdec(substr($color,1,2));
			$g = hexdec(substr($color,3,2));
			$b = hexdec(substr($color,5,2));
			$color = imagecolorallocate($img, $r, $g, $b);
			$font = self::$font_dir.$fontname.'.ttf';
			imagettftext($img, $size, 0, $x, $y, $color, $font, $text);
			$parts = explode(".",$out);
			$ext = $parts[count($parts)-1];
			if($ext == "jpg") $ext = "jpeg";
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $img, $out);
			imagedestroy($img);
		}catch (Exception $e){
			echo $e->getMessage();
		}
	}

	/**
	 * $out : output image file , relative path under self::$export_dir
	 * $bg : bg image file , relative path under self::$import_dir
	 * $objects : array of texts and images
	 * 			[
	 * 				{
	 * 					type : text  //neccesary , text | image
	 * 					text : "this is an example",	//text only
	 * 					x	 :	10, 	// image/text x-coordination upon bg
	 * 					y    :  15,		// image/text y-coordination upon bg
	 * 					size : 	12,		// text only
	 * 					font :  "Silom"	// font name under self::$font_dir, without ".ttf"
	 * 					color:  "#FFFFFF"	// string of color hex value
	 * 				},
	 * 				{
	 * 					type : image  	//neccesary , text | image
	 * 					url  : "http://www.google.co.jp/logos/2011/veterans_day-2011-hp.jpg" //target url
	 * 					x	 :	10, 	// image/text x-coordination upon bg
	 * 					y    :  15,		// image/text y-coordination upon bg
	 * 					w 	 :  50,		// image only, new width of target, optional
	 * 					h 	 :  40		// image only, new height of target, optional
	 * 				}
	 * 			]
	 *
	 * */
	public static function combine($out, $bg, $objects){
		self::preload();
		try{
			$parts = explode(".",$bg);
			$ext = $parts[count($parts)-1];
			if($ext == "jpg") $ext = "jpeg";
			$func_name = "imagecreatefrom".$ext;
			$bg = str_starts($bg, "/")? $bg:self::$import_dir.$bg;
			$out = str_starts($out, "/")? $out:self::$export_dir.$out;
			$img = call_user_func($func_name, $bg);
			foreach ($objects as $obj){
				$type = $obj["type"];
				$x = isset($obj["x"])?$obj["x"]:0;
				$y = isset($obj["y"])?$obj["y"]:0;
				if($type=="image"){
					$url = $obj["url"];
					if(!isset($url)||empty($url))
						continue;
					$full_path =str_starts($url, "/")||Strings::startsWith($url, "http")? $url:self::$import_dir.$url;
					$parts = explode(".",$full_path);
					$ext_target = $parts[count($parts)-1];
					if($ext_target == "jpg") $ext_target = "jpeg";
					$func_name = "imagecreatefrom".$ext_target;
					$target = call_user_func($func_name, $full_path);
					$w = $obj["w"];
					$h = $obj["h"];
					list($width, $height) = getimagesize($full_path);
					if($width==0 || $height==0)
						continue;
					if($w==null) $w=$width;
					if($h==null) $h=$height;
					imagecopyresized($img, $target, $x, $y, 0, 0, $w, $h, $width, $height);
					imagedestroy($target);
				}else if($type=="text"){
					$color = isset($obj["color"])?$obj["color"]:"#000000";
					$r = hexdec(substr($color,1,2));
					$g = hexdec(substr($color,3,2));
					$b = hexdec(substr($color,5,2));
					
					$color = imagecolorallocate($img, $r, $g, $b);
					$fontname = isset($obj["font"])?$obj["font"]:"Verdana";
					$size = isset($obj["size"])?intval($obj["size"]):9;
					$font = str_replace(" ","_", self::$font_dir.$fontname.'.ttf');
					$text = $obj["text"];
					$rows = $obj["row"];
					$cols = $obj["col"];
					
					$lines = explode('|', wordwrap($text, $cols, '|'));
					$lineNums = count($lines);
					
					$align = $obj["align"];

					if($rows>count($lines))
						$rows = count($lines);
					
					for ($idx = 0; $idx < $rows; $idx ++){
						$xo = $x;
						//$lineheight = $size*2;
						$line = $lines[$idx];
						if($idx+1 == $rows){//last row
							$letters = strlen($line);
							if($letters>$cols)
								$line = substr($line,0,$cols-2)+"...";
							
						}
						$bbox = imagettfbbox($size, 0, $font, $line);
						if($align=="right"){
								//recalculate x
								// letter width ~= $size
							$xo = $x - $bbox[2];
						}
						$lineHeight = abs($bbox[5]);
						imagettftext($img, $size, 0, $xo, $y, $color, $font, $line);
						$y += $lineHeight+2;
					}
				}
			}
			$parts = explode(".",$out);
			$ext = $parts[count($parts)-1];
			if($ext == "jpg") $ext = "jpeg";
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $img, $out);
			imagedestroy($img);
		}catch(Exception $e){
			echo $e->getMessage();
		}
	}

	private static function preload(){
		//$CONF;
		self::$import_dir = get_conf("GD_IMPORT_DIR");
		self::$export_dir = get_conf("GD_EXPORT_DIR");
		self::$font_dir   = get_conf("GD_FONT_DIR");
	}
	
	/**
	 * @example : make thumbnail
	 * 
	 * $image = $ext=="png"?imagecreatefrompng($imagePath): imagecreatefromjpeg( $imagePath );
	   GDs::resize ($image, $ext, $imagePath, 80);
	   
	 * */
	static function resize($img, $ext, $path, $newWidth ) {
		exec("rm -f $path");
		$width = imagesx( $img );
	    $height = imagesy( $img );
	 	$newHeight = floor( $height * ( $newWidth / $width ) );
	 	
	 	$tmp_img = imagecreatetruecolor( $newWidth, $newHeight );
	
	 	$black = imagecolorallocate($tmp_img, 0, 0, 0);

		// Make the background transparent
		imagecolortransparent($tmp_img, $black);
		
	     // copy and resize old image into new image 
	    imagecopyresized( $tmp_img, $img, 0, 0, 0, 0, $newWidth, $newHeight, $width, $height );
	    
	    // save thumbnail into a file
	    if($ext=="png"){
	    	imagepng($tmp_img, $path);
	    }else{
	    	imagejpeg($tmp_img, $path);
	    }


	}

}