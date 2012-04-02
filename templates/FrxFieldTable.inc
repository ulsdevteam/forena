<?php
require_once('FrxTemplate.inc');
class FrxFieldTable extends FrxTemplate {
	public function generate($xml, $config) {

		$div = $this->blockDiv($config);
		$columns = $this->columns($xml);
		$table = $this->addNode($div, 4, 'table', NULL, NULL, array('foreach' => '*'));
		if ($columns) foreach($columns as $col) {
			$tbody = $this->addNode($table, 6, 'tbody');
			$tr = $this->addNode($tbody, 8, 'tr');
			$th = $this->addNode($tr, 10, 'th', $col);
			$td = $this->addNode($tr, 10, 'td', '{' . $col . '}');
		}
	}
}