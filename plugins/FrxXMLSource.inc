<?php
/**
	* Look at the FrxRender class to see a full list of
  * properties that can be used here.
  */
class FrxXMLSource extends FrxRenderer {
	public function render() {
	  $output = '';
		$node = $this->reportDocNode;
		$xml = FrxData::instance()->currentContext();
		if ($xml && is_callable($xml, 'asXML')) $output = htmlspecialchars($xml->asXML());
		return $output;
	}
}