<?php
/**
 * @file FrxXLSXWordDoc.inc
 * Excel Document export.
 * @author davidmetzler
 *
 */

class FrxXLSXDoc extends FrxDocument {
  public function __construct() {
    $this->content_type = 'application/msexcel';
  }

  public function render($r, $format, $options = array()) {
    $phpspreadsheet = forena_library_file('PHPSpreadSheet');
    require $phpspreadsheet;
    $title= $r->title;
    $spreadsheet = new PhpOffice\PhpSpreadsheet\Spreadsheet();
    $body = $r->html;
    $doc = new DOMDocument();
    $doc->strictErrorChecking = FALSE;
    libxml_use_internal_errors(true);
    @$doc->loadHTML($body);
    libxml_clear_errors();
    $xml = simplexml_import_dom($doc);
    $tables = $xml ? $xml->xpath('//table') : array();
    $count = 1;
    if ($tables) foreach ($tables as $table) {
      $count ++;
      $rows = $table->xpath('descendant::tr');
      $rowCounter = 1;
      $columnCounter = 0;
      $spreadsheet->createSheet();
      $spreadsheet->getSheet($count - 1)->setCellValue('A1',$title);  //insert report title in first row and column
      $rowCounter++;
      if ($rows) foreach ($rows as $row) {
	$rowCounter++;
        foreach ($row as $column) {
          $class = @(string)$column['class'];
          $classes = explode(' ', $class);
          $text_class = array_search('XLSText', $classes)!==FALSE;
          $value = $column->asXML();
          $value = strip_tags($value);
          $tval = trim($value);
          // Find if it contains invalid number characters
          $non_numeric_chars = trim($value, ' +-.,0123456789');
          // Determine if it contains +- in the interior
          // Zero is ok here bu
          $inner_symbols = FALSE;
          if (strpos($tval, '+') || strpos($tval, '-') || strpos($tval, ' ')) $inner_symbols =  TRUE;
          if (substr_count($tval, '.') > 1) $inner_symbols = TRUE;
          if (empty($non_numeric_chars) && trim($value)!=='' && !$inner_symbols && !$text_class) {
	    $spreadsheet->getSheet($count - 1)->setCellValueByColumnAndRow($columnCounter, $rowCounter, $tval);
          }
          else {
            $spreadsheet->getSheet($count - 1)->setCellValueByColumnAndRow($columnCounter, $rowCounter, $value);
          }	    
	  $columnCounter++;
        }
	$columnCounter = 0;
      }
    }
    $sheetIndex = $spreadsheet->getIndex(
        $spreadsheet->getSheetByName('Worksheet')
    );
    $spreadsheet->removeSheetByIndex($sheetIndex);
    $directory = file_directory_temp();
    $writer = new PhpOffice\PhpSpreadsheet\Writer\Xlsx($spreadsheet);
    $path = request_path();  // will print "url"
    $path1 = strripos($path,'/');
    $pathLength = strlen($path);
    $path = substr($path, $path1 + 1, $pathLength);
    $path = substr($path, 0, strripos($path, '.')).".xlsx";
    $writer->save($directory."/".$path);
    $fileData = file_get_contents($directory."/".$path);
    unlink($directory."/".$path);
    return $fileData;
  }

  public function output(&$output) {
    $output = $this->convertCharset($output);
    parent::output($output);
    return TRUE;
  }
}
