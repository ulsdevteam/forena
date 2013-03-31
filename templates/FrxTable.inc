<?php
/**
 * @file FrxTable
 * Template that lays out a report in a particular table format.
 */
require_once('FrxTemplate.inc');
class FrxTable extends FrxTemplate {
  public function generate($xml, $config) {
    $block = @$config['block'];
    $id = @$config['id'];
    if ($block) {
      $id = $this->idFromBlock($block);
      $config['id'] = $id . '_block';
    }
    $config['class'] = @$config['class'] ? $config['class'] . ' FrxTable' : 'FrxTable';
    $div = $this->blockDiv($config);
    $columns = $this->columns($xml);

    $attrs = array('foreach' => '*');
    if (!$columns) {
      $columns = $this->columns($xml, '/*');
      $attrs = array();
    }
    $table = $this->addNode($div, 4, 'table');
    $thead = $this->addNode($table, 6, 'thead');
    $throw = $this->addNode($thead, 8, 'tr');
    $tbody = $this->addNode($table, 6, 'tbody');
    $tdrow = $this->addNode($tbody, 8, 'tr', NULL, array('id' => $id),$attrs);
    if ($columns) foreach ($columns as $col) {
      $this->addNode($throw, 10, 'th', $col);
      $this->addNode($tdrow, 10, 'td', '{' . $col . '}');
    }
  }
}