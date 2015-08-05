<?php
/**
 *	@file: GDs.inc
 *	@author: Soyoes 2011/07/14
 *	@uses: Combile, resize, drawText on image.
 *	@conf:
 *     Conf::$gd_import_path
 *     Conf::$gd_export_path
 *     Conf::$gd_font_path
 * 
 *****************************************************************************/

class GD{

	/**
	 * php_gd2
	 * @param out : relative export path under Conf::$gd_export_path
	 * @param bg : background img's relative path Conf::$gd_import_path
	 * @param fg : forground img file 's relative path Conf::$gd_import_path
	 * @param x, y target's coordinations upon background image
	 * @param w, h target's new size
	 *
	 * @example: self::merge('out.gif', 'bg.gif', 'rune/target.gif', 30, 20, 80,60); //merge out.gif to bg.gif at (30,20) of bg.gif , and enlarge/shrink target.gif to 80*60
	 * @example: self::merge('out1.gif', 'bg.jpg', 'item1.gif', 100, 20);
	 * */
	public static function merge($out, $bg, $fg, $x=0, $y=0, $w=null, $h=null){
		try{
			$imp_dir = property_exists('Conf','gd_import_path')? Conf::$gd_import_path:'';
			$exp_dir = property_exists('Conf','gd_export_path')? Conf::$gd_export_path:'';

			$bg  = str_starts($bg, '/')? $bg:$imp_dir.$bg;
			$fg  = str_starts($fg, '/')? $fg:$imp_dir.$fg;
			$out = str_starts($out,'/')? $out:$exp_dir.$out;
			$imgs = [];
			foreach ([$bg, $fg] as $fn){
				$ext = self::ext($fn);
				$func_name = "imagecreatefrom".$ext;
				$imgs[] = call_user_func($func_name, $filename);
			}
			list($width, $height) = getimagesize($fg);
			if($w==null) $w=$width;
			if($h==null) $h=$height;
			imagecopyresized($imgs[0], $imgs[1], $x, $y, 0, 0, $w, $h, $width, $height);
			$ext = self::ext($out);
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $imgs[0], $out);
			imagedestroy($imgs[0]);
			imagedestroy($imgs[1]);
		}catch (Exception $e){
			error_log("GD::merge ERR:".$e->getMessage());
		}
	}

	/**
	 * draw text on pic, default position=center, default color=black
	 * @$fontname : font name under Conf::$gd_font_path without ".ttf". for instance "arial"
	 * @$size	: font size of int value
	 * */
	public static function text($out, $bg, $text, $fontname, $fontsize, $color="#000000", $stroke=null, $strokeWidth=0, $x=-1, $y=-1){
		try{
			$imp_dir = property_exists('Conf','gd_import_path')? Conf::$gd_import_path:'';
			$exp_dir = property_exists('Conf','gd_export_path')? Conf::$gd_export_path:'';
			$font_dir= property_exists('Conf','gd_font_path')? 	Conf::$gd_font_path:'';

			$bg  = str_starts($bg, '/')? $bg:$imp_dir.$bg;
			$out = str_starts($out,'/')? $out:$exp_dir.$out;

			$ext = self::ext($bg);
			$func_name = "imagecreatefrom".$ext;
			$img = call_user_func($func_name, $bg);
			$r = hexdec(substr($color,1,2));
			$g = hexdec(substr($color,3,2));
			$b = hexdec(substr($color,5,2));
			$color = imagecolorallocate($img, $r, $g, $b);


			if($stroke){
				$r = hexdec(substr($stroke,1,2));
				$g = hexdec(substr($stroke,3,2));
				$b = hexdec(substr($stroke,5,2));
				$strokec = imagecolorallocate($img, $r, $g, $b);
			}

			$font = $font_dir.$fontname.'.ttf';

			$box = imagettfbbox($fontsize, 0, $font, $text);			
			$twa = abs($box[4] - $box[0]); //text width 
			$tha = abs($box[1] - $box[5]); //text height 
			$bgw = imagesx($img);
			$bgh = imagesy($img);

			$lines = explode("\n", $text);

			
			$ox = $x; $oy = $y;
			$rh = $tha/count($lines); //row height

			$i = 0; 
			foreach ($lines as $line) {
				$box = imagettfbbox($fontsize, 0, $font, $line);			
				$tw = abs($box[4] - $box[0]);
				$th = abs($box[1] - $box[5]);
				
				if($ox<0){//center
					$x = ($bgw-$tw)/2;
				}
				$y = $oy<0? ($bgh-$tha)/2+$i*$rh + ($rh-$th)/2 : $y + +$i*$rh + ($rh-$th)/2;

				if($stroke){
					for($c1 = ($x-abs($strokeWidth)); $c1 <= ($x+abs($strokeWidth)); $c1++)
				        for($c2 = ($y-abs($strokeWidth)); $c2 <= ($y+abs($strokeWidth)); $c2++)
				            imagettftext($img, $fontsize, 0, $c1, $c2, $strokec, $font, $line);
				    imagettftext($img, $fontsize, 0, $x, $y, $color, $font, $line);
				}	
				else
					imagettftext($img, $fontsize, 0, $x, $y, $color, $font, $line);			

				$i++;
			}

			$ext = self::ext($out);
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $img, $out);
			imagedestroy($img);
		}catch (Exception $e){
			error_log("GD::merge ERR:".$e->getMessage());
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
		try{
			$imp_dir = property_exists('Conf','gd_import_path')? Conf::$gd_import_path:'';
			$exp_dir = property_exists('Conf','gd_export_path')? Conf::$gd_export_path:'';
			$font_dir= property_exists('Conf','gd_font_path')? 	Conf::$gd_font_path:'';

			$bg  = str_starts($bg, '/')? $bg:$imp_dir.$bg;
			$out = str_starts($out,'/')? $out:$exp_dir.$out;

			$ext = self::ext($bg);
			$func_name = "imagecreatefrom".$ext;
			$img = call_user_func($func_name, $bg);

			foreach ($objects as $obj){
				$type = $obj["type"];
				$x = isset($obj["x"])?$obj["x"]:0;
				$y = isset($obj["y"])?$obj["y"]:0;
				if($type=="image"){
					$url = $obj["url"];
					if(!isset($url)||empty($url))
						continue;
					$full_path =str_starts($url, "/")||str_starts($url, "http")? $url:$imp_dir.$url;
					$ext_target = self::ext($full_path);
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
					$font = str_replace(" ","_", $font_dir.$fontname.'.ttf');
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
			$ext = self::ext($out);
			header("Content-Type: image/".$ext);
			fs_mkdir($out);
			call_user_func("image".$ext, $img, $out);
			imagedestroy($img);
			return true;
		}catch(Exception $e){
			return false;
			error_log($e->getMessage());
		}
	}

	
	
	/**
	 * @example : make thumbnail
	 *  GDs::resize ("out.jpg", $image, 80);
	 * */
	static function resize($out, $img, $newWidth=null, $newHeight=null) {
		try{
			$imp_dir = property_exists('Conf','gd_import_path')? Conf::$gd_import_path:'';
			$exp_dir = property_exists('Conf','gd_export_path')? Conf::$gd_export_path:'';
			
			$img = str_starts($img, '/')? $img:$imp_dir.$img;
			$out = str_starts($out,'/')? $out:$exp_dir.$out;

			exec("rm -f $out");

			$ext = self::ext($img);
			$func_name = "imagecreatefrom".$ext;
			$img = call_user_func($func_name, $img);

			$width = imagesx( $img );
		    $height = imagesy( $img );

		    if($newWidth)
		 		$newHeight = floor( $height * ( $newWidth / $width ) );
		 	else if($newHeight)
		 		$newWidth = floor( $width * ( $newHeight / $height ) );
		 	
		 	$tmp_img = imagecreatetruecolor( $newWidth, $newHeight );
		
		 	$black = imagecolorallocate($tmp_img, 0, 0, 0);

			// Make the background transparent
			imagecolortransparent($tmp_img, $black);
			
		     // copy and resize old image into new image 
		    imagecopyresized( $tmp_img, $img, 0, 0, 0, 0, $newWidth, $newHeight, $width, $height );
		    
		    // save thumbnail into a file
		    $func_name = "image".$ext;
			call_user_func($func_name, $tmp_img, $out);
			imagedestroy($tmp_img);
			imagedestroy($img);
			return true;
		}catch(Exception $e){
			return false;
			error_log("ERR GD::resize() ".$e->getMessage());
		}
	    
	}

	/**
	 * @example : crop 
	 *  GDs::crop ("out.jpg", $image, 0,100,200,200);
	 * */
	static function crop($out, $img, $x, $y, $w, $h) {
		try{
			$imp_dir = property_exists('Conf','gd_import_path')? Conf::$gd_import_path:'';
			$exp_dir = property_exists('Conf','gd_export_path')? Conf::$gd_export_path:'';
			
			$img = str_starts($img, '/')? $img:$imp_dir.$img;
			$out = str_starts($out,'/')? $out:$exp_dir.$out;

			exec("rm -f $out");

			$ext = self::ext($img);
			$func_name = "imagecreatefrom".$ext;
			$img = call_user_func($func_name, $img);

			

			$ww = imagesx( $img );
		    $hh = imagesy( $img );

		    $tmp_img = imagecreatetruecolor( $w, $h );
		
		 	$black = imagecolorallocate($tmp_img, 0, 0, 0);

			// Make the background transparent
			imagecolortransparent($tmp_img, $black);
			
		     // copy and resize old image into new image 
		    if($ww>$hh){
		    	imagecopyresized( $tmp_img, $img, 0, 0, $x, $y, $w, $h, $hh/$h*$w, $hh/$h*$h);	
		    	// imagecopy( $tmp_img, $img, 0, 0, $x, $y, $hh/$h*$w, $hh/$h*$h);	
		    }else{
		    	imagecopyresized( $tmp_img, $img, 0, 0, $x, $y, $w, $h, $ww/$w*$w, $ww/$w*$h);	
		    	// imagecopy( $tmp_img, $img, 0, 0, $x, $y, $ww/$w*$w, $ww/$w*$h);	
		    }
		    
		    // save thumbnail into a file
		    $func_name = "image".$ext;
			call_user_func($func_name, $tmp_img, $out);

			imagedestroy($tmp_img);
			imagedestroy($img);
			return true;
		}catch(Exception $e){
			return false;
			error_log("ERR GD::crop() ".$e->getMessage());
		}
	    
	}

	static function ext($fn){
		$ext = end(explode(".",$fn));
		return $ext == "jpg" ? "jpeg":$ext;
	}

	/**
	 * @param  [type] $photo    base64 decoded string
	 * @param  [type] $out_dir  e.g. /var/www/html/export/
	 * @param  [type] $out_file e.g. 121343434 (without .jpg)
	 * @param  [type] $out_ext  e.g. jpg
	 */
	static function export($photo, $out_dir, $out_file, $out_ext="jpg"){
		$imgData = str_starts($photo,"data")? base64_decode(explode(',', $photo)[1]):base64_decode($photo);
		$src = imagecreatefromstring($imgData);
		error_log("GD::export out_dir=$out_dir");
		fs_mkdir($out_dir);
		$outf = "$out_dir/$out_file.$out_ext";
		if($out_ext=="jpg")
			imagejpeg($src,$outf,75);
		else if($out_ext=="png")
			imagepng($src,$outf);
		self::resize("$out_dir/$out_file"."_s.jpg", $outf, null, 120);
		imagedestroy($src);	
	}

}