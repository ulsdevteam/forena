<?php
/**
 * Base class for Frx custom renderers
 * @author davidmetzler
 *
 */
class FrxRenderer {
	public $domNode;
	public $teng;
	public $xmlNode;
	public $frxAttributes;
	public $htmlAttributes;
	public $attrib_text;    //Property of
	public $dataProvider;
	public $name;
	public $id;


	public function initNode($domNode, $teng) {
		$this->domNode = $domNode;
    $this->dataProvider = FrxData::instance();
    $this->xmlNode = simplexml_import_dom($domNode);
    $node = $this->xmlNode;
    $this->name = $node->getName();
    $this->htmlAttributes = $node->attributes();
    $this->id = (string)$this->htmlAttributes['id'];
    $this->frxAttributes = $node->attributes(FRX_NS);
    $this->teng = $teng;
	}

	public function replaceTokens($text, $raw_mode=FALSE) {
		return $this->teng->replace($text, $raw_mode);
	}

	public function render($domNode, $teng) {
		$this->initNode($domNode, $teng);
		// Get the data that we're working with
		$xml = $this->dataProvider->getCurrentContext();
		$node = $this->nodes;
    // We can render so lets do it.
    $text = $this->xmlNode->asXML();
    $o = $this->teng->replace($text);
    return $o;
	}


}