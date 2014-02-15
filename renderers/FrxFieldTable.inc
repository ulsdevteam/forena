<?php
/**
 * @file FrxFieldTable
 * Template that displays a table of column/value vertically.
 */
class FrxFieldTable extends FrxRenderer {
  public $templateName = 'Table of Fields';
  public function generate($xml, $config) {
    $config['foreach'] = '*';
    $config['class'] = 'FrxFieldTable';
    $div = $this->blockDiv($config);
    $columns = $this->columns($xml);
    $table = $this->setFirstNode($div, 4, 'table');
    $this->removeChildren($table);
    $r=0;
    if ($columns) foreach ($columns as $col => $label) {
      $r++;
      $tbody = $this->addNode($table, 6, 'tbody');
      $tr = $this->addNode($tbody, 8, 'tr');
      $th_attrs = array();
      if ($r==1) {
        $th_attrs = array('style' => 'width: 25em;');
      }

      $th = $this->addNode($tr, 10, 'th', $label);
      $td = $this->addNode($tr, 10, 'td', '{' . $col . '}');
    }
  }
}