<?php 

trait csvProc{
    public function csv_to_array($filename='', $delimiter=',')
    {
        if(!file_exists($filename) || !is_readable($filename))
            return FALSE;

        $header = NULL;
        $data = array();
        if (($handle = fopen($filename, 'r')) !== FALSE)
        {
            while (($row = fgetcsv($handle, 1000, $delimiter)) !== FALSE)
            {
                if(!$header)
                    $header = $row;
                else
                    $data[] = array_combine($header, $row);
        }
            fclose($handle);
        }
        return $data;
    }

    function parse_csv ($string='', $row_delimiter=PHP_EOL, $delimiter = "," , $enclosure = '"' , $escape = "\\")
    {
        $rows = array_filter(explode($row_delimiter, $string));
        $header = NULL;
        $data = array();

        foreach($rows as $row)
        {
            $row = str_getcsv ($row, $delimiter, $enclosure , $escape);

            if(!$header)
                $header = $row;
            else
                $data[] = array_combine($header, $row);
        }

        return $data;
    }
}

?>