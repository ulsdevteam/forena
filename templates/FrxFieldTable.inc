<?php
/**
 * @file FrxFieldTable
 * Template that displays a table of column/value vertically.
 */
require_once('FrxTemplate.inc');
class FrxFieldTable extends FrxTemplate {
  public function generate($xml, $config) {
    $config['foreach'] = '*';
    $config['class'] = 'FrxFieldTable';
    $div = $this->blockDiv($config);
    $columns = $this->columns($xml);
    $table = $this->addNode($div, 4, 'table', NULL, NULL, NULL);
    $r=0;
    if ($columns) foreach ($columns as $col) {
      $r++;
      $tbody = $this->addNode($table, 6, 'tbody');
      $tr = $this->addNode($tbody, 8, 'tr');
      $th = $this->addNode($tr, 10, 'th', $col);
      if ($r==1) {
        $th->setAttribute('width', '300');
      }
      $td = $this->addNode($tr, 10, 'td', '{' . $col . '}');
    }
  }
}