<?php
/**
 * @file FrxCSVDoc.inc
 * CSV Document export.
 * @author davidmetzler
 *
 */
class FrxCSVDoc extends FrxDocument {

  public function __construct() {
    $this->content_type = 'application/csv';
  }

  /**
   * Fill columns with rowspan tags.
   * @param unknown_type $colspan
   */
  private function fillColumns($colspan) {
    $f = '';
    for ($i=2; $i<=$colspan; $i++) {
      $f .= ',';
    }
    return $f;
  }

  public function render($r, $format, $options = array()) {
    $doc = $this->check_markup($r->html);
    $dom = new DOMDocument();
    $dom->strictErrorChecking = FALSE;
    $xmlBody = '<?xml version="1.0" encoding="UTF-8"?>' . $doc;
    @$dom->loadHTML($xmlBody);
    $xml = simplexml_import_dom($dom);
    $output = '';
    $rows = $xml->xpath('//tr');
    $rowspans = array();
    if ($rows) foreach ($rows as $row) {
      $c = 0;
      foreach ($row as $column) {
        $c++;
        if (@$rowspans[$c]) {
          $cont = TRUE;
          while ($rowspans[$c] && $cont) {
            $rowspans[$c]--;
            $output .= ',';
            $c++;
          }
        }
        $value = $column->asXML();
        $value = strip_tags($value);
        $value = decode_entities($value);
        $value = str_replace('"', '""', $value);
        $value = str_replace(array("\n"), '', $value);
        $value =  '"' . $value . '",';
        $output .= $value;
        // Add Column span elements
        if ((int)$column['colspan'] > 1) {
          for ($i=2; $i<=(int)$column['colspan']; $i++) {
            $c++;
            $output .= ',';
          }
        }
        // Check to see if we have some rowspans that we need to save
        if ((int)$column['rowspan'] > 1) {
          $rowspans[$c] = (int)$column['rowspan'] - 1;
        }
      }
      $output .= "\n";
    }
    return $output;
  }


  public function output($output) {
    parent::output($this->convertCharset($output));
    return TRUE;
  }
}

