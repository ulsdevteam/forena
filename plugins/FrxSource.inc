<?php
/**
	* Look at the FrxRender class to see a full list of
  * properties that can be used here.
  */
class FrxSource extends FrxRenderer {
	public function render($domNode, $teng) {
		$this->initNode($domNode, $teng);
		$output = htmlspecialchars($this->xmlNode->asXML());
		return $output;
	}
}