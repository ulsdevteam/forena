<?php
class FrxBlockEditor {
  public $block = array();
  public $block_name;
  public $modified;
  public $provider;
  private $new_block = array(
    'type' => 'sql',
    'file' => '',
    'access' => '',
    'source' => '',
    'info' => array(),
  );

  public function __construct($block_name = '') {
    if ($block_name) {
      $this->load($block_name);
    }
  }

  /**
   * Block loader method.
   * @param  $block_name
   */
  public function load($block_name) {
    $block_name = str_replace('.', '/', $block_name);
    @list($provider, $path) = explode('/', $block_name, 2);
    $this->provider = $provider;
    $this->block_name = $block_name;
    if (!isset($_SESSION['forena_query_editor'][$block_name])) {
      $block = Frx::RepoMan()->loadBlock($block_name);
      $this->modified = FALSE;
      if (!$block) {
        $block = $this->block = $this->new_block;
        $this->update($this->block);
      }
    }
    else {
      $block = $_SESSION['forena_query_editor'][$block_name];
      drupal_set_message(t('All changes are stored temporarily.  Click Save to make your changes permanent.  Click Cancel to discard your changes.'), 'warning', FALSE);
      $this->modified = TRUE;
    }
    $this->block = $block;
    return $block;
  }


  /**
   * Update the block data.
   * @param unknown $block
   * @return FrxBlockEditor
   */
  public function update($block = array()) {
    if ($block) {
      // Make sure we don't put any invalid values
      $block = array_intersect_key($block, $this->block);
      $block = array_merge($this->block, $block);
      $same = $block['file']== $this->block['file'] && $block['access']==$this->block['access'];
      // UPdate the block;
      if (!$same) {
        $this->block = $block;
        $_SESSION['forena_query_editor'][$this->block_name] = $this->block;
        $this->modified = TRUE;
      }
    }
    return $this;
  }

  /**
   * Saves the data block
   */
  public function save() {
    Frx::RepoMan()->saveBlock($this->block_name, $this->block);
    unset($_SESSION['forena_query_editor'][$this->block_name]);
    drupal_get_messages('warning');
    drupal_set_message(t('Saved Changes'));
    return $this;
  }


  /**
   * Cancel the editing event.
   */
  public function cancel() {
    unset($_SESSION['forena_query_editor'][$this->block_name]);
    drupal_get_messages('warning');
    $this->modified = FALSE;
  }

  /**
   * Return repository.
   * @return FrxDataSource
   */
  public function repos() {
    return Frx::RepoMan()->repository($this->provider);
  }

  /**
   * Rename the exisiting block.
   * @param unknown $name
   */
  public function rename($name) {
    $old_name = $this->$block_name;
    unset($_SESSION['forea_query_editor'][$old_name]);
    $this->$block_name = $this->provider . '/' . $name;
    $this->update($this->block);
  }

  public function preview($parms = array()) {
    $preview = '';
    FrxReportGenerator::instance()->alter_parameters('', $parms);
    $block = $this->block;
    if ($block['type'] == 'sql') {
      $sql = $block['file'];

      $data = Frx::RepoMan()->sqlData($this->provider, $sql, $parms);
    }
    else {
      Frx::Data()->push($parms, 'parm');
      Frx::RepoMan()->data($this->block_name);
      Frx::Data()->pop();
    }

    // Generate the template
    $t = Frx::Template('FrxTable');
    $config = array('class' => 'dataTable', 'id' => 'preview-data');
    if ($data && $t) {
      $t->generate($data, $config);
      $fields = $t->columns;
      if ($fields) $fields = array_combine($fields, $fields);
      if ($fields) {
        $report = $t->asXML();
        $rpt = new FrxReport($report, $data);
        $preview = $rpt->render('web', FALSE);
      }
      else {
        if (is_object($data) && method_exists($data, 'asXML')) {
          $preview = '<pre>' . htmlspecialchars($data->asXML()) . '</pre>';
        }
        else {
          $preview = '<pre>' .htmlspecialchars(print_r($data,1)) .'</pre>';
        }
      }
    }
    return $preview;

  }

}