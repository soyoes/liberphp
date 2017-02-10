<?php

/**

    when $s3_emulate is true:
        S3 emulation mode.
        Parameters $s3_emulate_path and $s3_emulate_url must be set.
        In this situation, files will be uploaded to a local folder, which is defined in $s3_emulate_path.
        Typically, $s3_emulate_path should be set to a folder which is under a web project's DocumentRoot,
        $s3_emulate_url should be set to a web path of the $s3_emulate_path
    when $s3_emulate is false:
        Files will be uploaded to S3 server.
        Parameters $s3_key, $s3_secret, $s3_bucket, $s3_public_access must be give.
    
    Add these to your Conf file

    static $s3_emulate          = false;
    static $s3_emulate_path     = "/var/www/html/example/webroot/images/tmp";
    static $s3_emulate_url      = "http://example.com/images/tmp";
    static $s3_bucket           = "example";
    static $s3_public_access    = true;
    static $s3_key              = "%S3_ACCESS_ID%";
    static $s3_secret           = "%S3_SECRET_KEY%";


*/


require "%PATH%/aws.phar";
use Aws\S3\S3Client;

/**
    AWS sdk for php: https://github.com/aws/aws-sdk-php
    aws.phar can be downloaded from https://github.com/aws/aws-sdk-php/releases/tag/2.7.13
*/

class S3 {

    private static $s3 = null;

    public static function init() {
        if (!isset(self::$s3)) {
            $s3 = S3Client::factory([
                "key" => Consts::$s3_key,
                "secret" => Consts::$s3_secret
            ]);

            $s3->registerStreamWrapper();
            self::$s3 = $s3;
        }

        return new S3();
    }

    private function __construct() {}

    public function delete($name) {
        return unlink("s3://" . Consts::$s3_bucket . "/" . $name);
    }

    public function download($name) {
        return file_get_contents("s3://" . Consts::$s3_bucket . "/" . $name);
    }

    /**
        params:
            $file   path or a base64 string.
            $name   Filename of the file. 
                    If empty, a uuid string will assign to the filename.
        return:
            The uploaded file's url.
    */
    public function upload($file, $name, $metadata = []) {
        preg_match("/^data:.+?\/(.+?);base64,.*/", $file, $matches);
        if (count($matches) == 2) {
            // file is a base 64 string
            $name = $name ? $name : strtolower(uniqid() . "." . $matches[1]);
        } else {
            // file is a path string
            $name = $name ? $name : uniqid();
        }

        $handler = fopen($file, "r");
        if (!$handler)
            return false;

        if (Consts::$s3_emulate) {
            $dest = fopen(Consts::$s3_emulate_path . DIRECTORY_SEPARATOR . $name, "w");
            stream_copy_to_stream($handler, $dest);

            return Consts::$s3_emulate_url . "/" . $name;
        } else {
            $option = [
                "Bucket" => Consts::$s3_bucket,
                "Key" => $name,
                "Body" => $handler,
            ];
            if (Consts::$s3_public_access)
                $option["ACL"] = "public-read";
            if ($metadata && count($metadata))
                $option["Metadata"] = $metadata;

            $result = self::$s3->putObject($option);

            return $result["ObjectURL"];
        }
    }
}
