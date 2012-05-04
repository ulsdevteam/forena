<?php
class FrxRptInclude extends FrxTemplate {


	public function generate($xml, $config) {
		$src = $this->extract('src', $key);

		$div = $this->blockDiv($config);
		$frx = array('src' => $src, 'renderer' => 'FrxInclude');
		$this->addNode($div, 4, 'div',null, $config, $frx);
	}
}