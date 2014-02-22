<?php
/**
 * @file FrxTable
 * Template that lays out a report in a particular table format.
 */
class FrxTable extends FrxRenderer {
  public $templateName = 'Simple Table';

  public function scrapeConfig() {
    $config=array();
    $this->extractTemplateHTML($this->reportDocDomNode, $config, array('table'));
    return $config;
  }

  public function generate($xml, $config) {
    $config['class'] = get_class($this);
    $block = @$config['block'];
    $id = @$config['id'];
    if ($block) {
      $id = $this->idFromBlock($block);
      $config['id'] = $id . '_block';
    }
    $config['class'] = @$config['class'] ? $config['class'] . ' FrxTable' : 'FrxTable';
    $div = $this->blockDiv($config);

    // PUt on the header
    $this->removeChildrenExcept($div, array('table'));
    if (isset($config['header']['value'])) $this->addFragment($div, $config['header']['value']);

    $columns = $this->columns($xml);

    $attrs = array('foreach' => '*');
    if (!$columns) {
      $columns = $this->columns($xml, '/*');
      $attrs = array();
    }
    $table = $this->setFirstNode($div, 4, 'table');
    $thead = $this->setFirstNode($table, 6, 'thead');
    $throw = $this->setFirstNode($thead, 8, 'tr');
    $tbody = $this->setFirstNode($table, 6, 'tbody');
    $tdrow = $this->setFirstNode($tbody, 8, 'tr', NULL, array('id' => $id),$attrs);
    $this->removeChildren($tdrow);
    $this->removeChildren($throw);
    if ($columns) foreach ($columns as $col => $label) {
      $this->addNode($throw, 10, 'th', $label);
      $this->addNode($tdrow, 10, 'td', '{' . $col . '}');
    }

    if (isset($config['footer']['value'])) $this->addFragment($div, $config['footer']['value']);
  }
}