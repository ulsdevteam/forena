<?php
class FrxRptInclude extends FrxTemplate {


	public function generate($xml, $config) {
		$src = $this->extract('src', $key);
		$div = $this->blockDiv($config);

	}
}