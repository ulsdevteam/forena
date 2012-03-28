<?php
class FrxTemplate {

  public $dom;
  public $document_root;
  public $simplexml;
  public $body;

  public $doc_prefix = '<?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE root [
    <!ENTITY nbsp "&#160;">
    ]>';


  public $xmlns = 'urn:FrxReports';

  public function __construct($xml_string = '') {


    $this->dom = new DOMDocument('1.0', 'UTF-8');
    $dom = $this->dom;

    // Load a new one or build the empty XML Document
    $xml_string = $this->doc_prefix . '<html xmlns:frx="urn:FrxReports"><head/><body/></html>';
    libxml_use_internal_errors();
    try {
      $dom->loadXML($xml_string);
    }
    catch (Exception $e) {
      forena_error('Invalid or malformed tmeplate document', '<pre>' .
        $e->getMessage() . $e->getTraceAsString() . '</pre>');
    }
    $this->body = $dom->getElementsByTagName('body')->item(0);
  }


	public function columns($xml, $path='/*/*') {
		    //create an array of columns
    $rows = $xml->xpath($path);
    $column_array = array();

    foreach ($rows as $columns) {
      foreach ($columns as $name => $value) {
        $column_array[$name] = $name;
      }
    }
		return $column_array;
	}

	/**
	 * Returns the section
	 * Enter description here ...
	 */
	public function config_form($config) {
		$form_ctl = array();
		$form_ctl['#tree'] = TRUE;
		$form_ctl['id'] = array(
		  '#type' => 'textfield',
		  '#title' => t('CSS id'),
		  '#description' => t('Specify a unique identifier for the id of this data'),
		  '#default_value' => @$config['id'],
		);

		$form_ctl['heading'] = array(
		  '#type' => 'textfield',
		  '#title' => 'Section Header',
		  '#description'
		);

		$form_ctl['description'] = array(
		  '#type' => 'textarea',
		  '#title' => 'Description',
		);
		return $form_ctl;
	}

  function addNode($cur_node, $indent=0, $tag='div', $value='', $attributes=array(), $frx_attributes=array()) {
  	$dom = $this->dom;
  	if ($indent!==FALSE) {
  	  $tnode = $dom->createTextNode("\n" . str_repeat(' ', $indent));
  	  $cur_node->appendChild($tnode);
  	}
    $pnode = $dom->createElement($tag, $value);
  	if ($attributes) foreach ($attributes as $key => $value) {
  		if ($value) {
  			$attr = $dom->createAttribute($key);
  			$attr->value = $value;
  			$pnode->appendChild($attr);
  		}
  	}
  	if ($frx_attributes) foreach ($frx_attributes as $key => $value) {
      if ($value) {
	      $attr = $dom->createAttributeNS($this->xmlns, $key);
	      $attr->value = $value;
	      $pnode->appendChild($attr);
      }
  	}
  	$cur_node->appendChild($pnode);
  	return $pnode;
  }


  function addText($cur_node, $text) {
  	$dom = $this->dom;
  	$tnode = $dom->createTextNode($text);
  	$cur_node->appendChild($tnode);
  	return $tnode;
  }

  /**
   *
   * Extract a configuration var removing it from the array
   * @param string $key attribute key for the data being extracted.
   * @param array $config
   */
  public function extract($key, &$config) {
  	$value = '';
  	if (isset($config[$key])) {
  		$value = $config[$key];
  		unset($config[$key]);
  	}
  	return $value;
  }

  /**
   *
   * Generate generic div tag.
   * @param unknown_type $config
   * @param unknown_type $text
   */
  public function blockDiv($config, $text='') {
    $body = $this->body;

  	$frx_attributes = array(
      'block' => @$config['block'],
      'foreach' => @$config['foreach'],
    );

    $attributes = array(
      'id' => @$config['id'],
      'class' => @$config['class'],
    );

    $node = $this->addNode($body, 2, 'div', $text, $attributes, $frx_attributes);
  	return $node;
  }

	/**
	 *
	 * Enter description here ...
	 * @param string $data_block
	 * @param SimpleXMLElement $xml
	 * @param array $config
	 */
	public function generate($xml , $config) {


    if (!@$config['foreach']) $config['foreach']='*';


    $columns = $this->columns($xml);
    $text = '';
    if ($columns) foreach ($columns as $col) {
    	$text .= ' {' . $col . '}';
    }
    $this->blockDiv($config, $text);

  }

  public function template() {
  	$body = $this->body;
  	$output = '';
  	foreach ($body->childNodes as $node) {
  		$output .= $this->dom->saveXML($node);
  	}
  	return $output;
  }


  public function asXML() {
  	return $this->dom->saveXML();
  }


}