<?php
// $Id$
/**
 * @file contains various methods for extending report
 * formating, layout, transformation and design.
 *
 */

class FrxControls {
  /**
   * @section
   * Below here are advertising methods
   */

  //date formats
  public function formats() {
    $formats = array('drupal_date_format' => 'Drupal Date',
                     'iso_date' => 'ISO Date',
                     'sprintf' => 'PHP sprintf()',
                     'number' => 'Number',
                     );
    return $formats;
  }

  //document transformations
  public function doc_types() {
    $doc_types = array('doc' => 'htmldocument',
                       'html' => 'htmldocument',
                       'csv' => 'csvdocument',
                       'xls' => 'xlsdocument',
    );
    return $doc_types;
  }

  //Report templates
  public function templates() {
    $templates = array('table' => 'table',
                       'vertical_list' => 'list',
                      );

    return $templates;
  }

  //Date format methods
  public function drupal_date_format($value, $format_str) {
     if (!$format_str) $format_str = 'small';
     switch ($format_str) {
       case 'medium':
         $type = $format_str;
         $format='';
         break;
        case 'small':
         $type = $format_str;
         $format='';
         break;
        case 'large':
         $type = $format_str;
         $format='';
         break;
       default:
         $type = 'custom';
         $format = $format_str;
     }
     return $value ? format_date($value, $type, $format): '';
  }

  public function iso_date($value, $format_str) {
    if ($value) $date = strtotime($value);
    return $this->drupal_date_format($date, $format_str);
  }

  public function sprintf($value, $format_str) {
    if ($value) {
      $value = sprintf($format_str, $value);
    }
    return $value;
  }

  public function number($value, $format_str) {

    $dec= 2;
    $dec_sep='.';
    $th_sep = ',';
    if ($format_str && !trim($format_str, '9.,$')) {
      //Determine the decimal places.
      $dec_pos = strrpos($format_str, $dec_sep);
      if ($dec_pos) {
        $dec = strlen($format_str) - $dec_pos -1;
      }
      else {
        $dec = 0;
       $dec_sep = '';
      }
    }

    if ($value && $this->is_number($value)) $value = number_format($value, $dec, $dec_sep, $th_sep);
    return $value;
  }

  private function is_number($value) {
           $non_numeric_chars = trim($value, ' +-.,0123456789');
          // Determine if it contains +- in the interior
          // Zero is ok here bu
          $inner_symbols = FALSE;
          if (strpos($value, '+') || strpos($value, '-') || strpos($value, ' ')) $inner_symbols =  TRUE;
          return (empty($non_numeric_chars) && trim($value)!=='' && !$inner_symbols) ? TRUE : FALSE;
  }

  //Document transformation methods
  public function htmldocument($body, $options) {
    $output .= '<html><head>';

    if ($options['css']) {
      $output .= '<style type="text/css">';
      $output .= $options['css'];
      $output .= '</style>';
    }
    $output .= '<title>' . $options['title'] . '</title></head><body class="forena-report">' . $body . '</body></html>';
    return $output;
  }

  public function csvdocument($body, $options) {
    $doc = new DOMDocument();
    $doc->strictErrorChecking = FALSE;
    $doc->loadHTML($body);
    $xml = simplexml_import_dom($doc);

    $rows = $xml->xpath('//tr');
    if ($rows) foreach ($rows as $row) {
      foreach ($row as $column) {
        $value = $column->asXML();
        $value = strip_tags($value);
        $value = str_replace('"', '""', $value);
        $value = str_replace(array("\n"), '', $value);
        $value =  '"' . $value . '",';
        $output .= $value;
      }
      $output .= "\n";
    }
    return $output;
  }



  public function xlsdocument($body, $options) {
    $output .= '<?xml version="1.0"?>' . "\n";
    $output .= '<?mso-application progid="Excel.Sheet"?>' . "\n";
    $output .= '<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet"' . "\n";
    $output .= '  xmlns:o="urn:schemas-microsoft-com:office:office"' . "\n";
    $output .= '  xmlns:x="urn:schemas-microsoft-com:office:excel"' . "\n";
    $output .= '  xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet"' . "\n";
    $output .= '  xmlns:html="http://www.w3.org/TR/REC-html40">' . "\n";
    $output .= '<Styles>' . "\n";
    $output .= '  <Style ss:ID="Default" ss:Name="Normal">' . "\n";
    $output .= '    <Alignment ss:Vertical="Bottom"/>' . "\n";
    $output .= '    <Borders/>' . "\n";
    $output .= '    <Font/>' . "\n";
    $output .= '    <Interior/>' . "\n";
    $output .= '    <NumberFormat/>' . "\n";
    $output .= '    <Protection/>' . "\n";
    $output .= '  </Style>' . "\n";
    $output .= '</Styles>' . "\n";

    $doc = new DOMDocument();
    $doc->strictErrorChecking = FALSE;
    $doc->loadHTML($body);
    $xml = simplexml_import_dom($doc);

    $tables = $xml->xpath('//table');
    $count = 1;
    if ($tables) foreach ($tables as $table) {
      $output .= '<Worksheet ss:Name="sheet' . ' ' . $count . '">' . "\n";
      $count ++;
      $output .= '  <Table>' . "\n";

      $rows = $table->xpath('descendant::tr');
      if ($rows) foreach ($rows as $row) {
        $output .= '    <Row>' . "\n";
        foreach ($row as $column) {
          $value = $column->asXML();
          $value = strip_tags($value);
          $tval = trim($value);
          // Find if it contains invalid number characters
          $non_numeric_chars = trim($value, ' +-.,0123456789');
          // Determine if it contains +- in the interior
          // Zero is ok here bu
          $inner_symbols = FALSE;
          if (strpos($tval, '+') || strpos($tval, '-') || strpos($tval, ' ')) $inner_symbols =  TRUE;
          if (empty($non_numeric_chars) && trim($value)!=='' && !$inner_symbols) {
            $output .= '      <Cell><Data ss:Type="Number">' . $tval . '</Data></Cell>' . "\n";
          }
          else {
            $output .= '      <Cell><Data ss:Type="String">' . $value . '</Data></Cell>' . "\n";
          }
        }
        $output .= '    </Row>' . "\n";
      }
      $output .= '  </Table>' . "\n";
      $output .= '  <WorksheetOptions xmlns="urn:schemas-microsoft-com:office:excel">' . "\n";
      $output .= '    <Selected/>' . "\n";
      $output .= '    <ProtectObjects>False</ProtectObjects>' . "\n";
      $output .= '    <ProtectScenarios>False</ProtectScenarios>' . "\n";
      $output .= '  </WorksheetOptions>' . "\n";
      $output .= '</Worksheet>' . "\n";
    }
    $output .= '</Workbook>';
    return $output;
 }

 //template methods
  public function table($columns, $data_block, $clause='') {
    if ($clause) $clause = 'frx:clause ="' . $clause . '"';
    $xml = '<div frx:block="' . $data_block . '" ' . $clause . '>';
    $xml .= '<table>';
    $xml .= '<thead>';
    $xml .= '<tr>';
    //Create the headers
    foreach ($columns as $header) {
     $xml .= '<th>' . $header . '</th>';
    }
    $xml .= '</tr>';
    $xml .= '</thead>';
    $xml .= '<tbody>';
    $xml .= '<tr frx:foreach="*">';
    foreach ($columns as $column) {
      $xml .= '<td>{' . $column . '}</td>';
    }
    $xml .= '</tr>';
    $xml .= '</tbody>';
    $xml .= '</table>';
    $xml .= '</div>';
    return $xml;
  }

  public function vertical_list($columns, $data_block, $clause) {
    if ($clause) $clause = 'frx:clause ="' . $clause . '"';
    $xml .= '<div frx:block="' . $data_block . '" ' . $clause . '>';
    $xml .= '<table frx:foreach="*">';
    $xml .= '<tbody>';

    foreach ($columns as $column) {
      $xml .= '<tr >';
      $xml .= '<th>' . $column . '</th><td>{' . $column . '}</td>';
      $xml .= '</tr>';
    }

    $xml .= '</tbody>';
    $xml .= '</table>';
    $xml .= '</div>';

    return $xml;
  }
}