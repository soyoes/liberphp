<?php

require "%PATH%/aws.phar";
use Aws\S3\S3Client;

/**
    AWS sdk for php: https://github.com/aws/aws-sdk-php
    aws.phar can be downloaded from https://github.com/aws/aws-sdk-php/releases/tag/2.7.13
*/
class S3 {
    public static function s3_conn() {
        $s3 = S3Client::factory([
            "key" => Consts::$s3_key,
            "secret" => Consts::$s3_secret
        ]);

        return $s3;
    }

    /**
        params:
            $file   path or a base64 string.
            $name   Filename of the file. 
                    If empty, a uuid string will assign to the filename.
        return:
            The uploaded file's url.
    */
    public static function upload($file, $name, $metadata = []) {
        preg_match("/^data:.+?\/(.+?);base64,.*/", $file, $matches);
        if (count($matches) == 2) {
            // file is a base 64 string
            $name = $name ? $name : strtolower(str_replace("-", "", Utils::uuid()) . "." . $matches[1]);
        } else {
            // file is a path string
            $name = $name ? $name : Utils::uuid();
        }

        $handler = fopen($file, "r");
        if (!$handler)
            return false;

        if (Consts::$s3_emulate) {
            $dest = fopen(Consts::$s3_emulate_path . DIRECTORY_SEPARATOR . $name, "w");
            stream_copy_to_stream($handler, $dest);

            return Consts::$s3_emulate_url . "/" . $name;
        } else {
            $s3 = self::s3_conn();
            
            $option = [
                "Bucket" => Consts::$s3_bucket,
                "Key" => $name,
                "Body" => $handler,
            ];
            if (Consts::$s3_public_access)
                $option["ACL"] = "public-read";
            if ($metadata && count($metadata))
                $option["Metadata"] = $metadata;

            $result = $s3->putObject($option);

            return $result["ObjectURL"];
        }
    }
}
