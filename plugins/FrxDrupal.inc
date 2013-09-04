<?php
/**
 * @file
 * Provides data blocks for native drupal connections using the default
 * drupal connections.
 *
 */
class FrxDrupal extends FrxDataSource {
  /**
   * Implements hooks into the drupal applications
   */

  private $database = 'default';

  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path, $name) {
    parent::__construct($conf, $repos_path, $name);
    if (@$conf['database'] && @$conf['database'] != 'default')  {
      $this->database = $conf['database'];
    }
    // Set up the stuff required to translate.
    $this->te = new FrxSyntaxEngine(FRX_SQL_TOKEN, ':', $this);
  }
  /**
   * Get data based on file data block in the repository.
   *
   * @param String $block_name
   * @param Array $parm_data
   * @param Query $subQuery
   */
  public function sqlData($sql, $options = array()) {
    if ($this->database != 'default') {
      db_set_active($this->database);
    }
    // Load the block from the file
    $xml ='';
    $sql = $this->te->replace($sql);
    $rs = db_query($sql);


    $xml = new SimpleXMLElement('<table/>');
    foreach ($rs as $data) {
      $row_node = $xml->addChild('row');
      /* If we have asked for one, lets load the entities based on
         the entity type and entity id that is specified in the block
         comments. */
      if (@$options['ENTITY_TYPE']  && @$options['ENTITY_ID'] ) {
        $id_key = $options['ENTITY_ID'];
        $id = $data->$id_key;
        if ($id) {
          $type = $options['ENTITY_TYPE'];
          $e = entity_load($type, (array) $id);
          $lang = isset($e->language) ? $e->language : 'und';
          if ($e) foreach ($e[$id] as $key => $val) if ($val) {
            if (strpos($key, 'field_') === 0) {
              //$fields = field_get_items('node', $node, $key);
              $field = field_view_field($type, $e, $key);
              $field['#theme'] = array('forena_inline_field');
              $value  = drupal_render($field);
              $f = $row_node->addChild($key, $value);
              if (isset($field['#field_type'])) $f['type'] = $field['#field_type'];
              if (isset($field['#field_name'])) $f['name'] = $field['#field_name'];
            } else if (is_array($val) && isset($val[$lang])) {
              dpm('in here');
              $tmp = $val[$lang][0];
              if (isset($tmp['safe_value'])) {
                $row_node->addChild($key, $tmp['safe_value']);
              } else if (isset($tmp['value'])) {
                $row_node->addChild($key, $tmp['value']);
              }
            } else if (is_scalar($val)) {
              $row_node->addChild($key, $val);
            }
          }

        }
      } else {
        foreach ($data as $key => $value) {
          $row_node->addChild($key, htmlspecialchars($value));
          //$row_node->$key = $value;
        }
      }
    }


    if ($this->database != 'default') {
      db_set_active();
    }
    if ($this->debug) {
      $d = ($xml) ? htmlspecialchars($xml->asXML()) : '';
      $this->debug('SQL: ' . $sql, '<pre> SQL:' . $sql . "\n XML: " . $d . "\n</pre>");
    }
    return $xml;
  }

  /**
   * Implement custom SQL formatter to make sure that strings are properly escaped.
   * Ideally we'd replace this with something that handles prepared statements, but it
   * wouldn't work for
   *
   * @param unknown_type $value
   * @param unknown_type $key
   * @param unknown_type $data
   */
  public function format($value, $key, $data) {
    $db = Database::getConnection('default');
    if ($db) {
      if ($value==='' || $value===NULL)
      $value = 'NULL';
      else {
        if (is_array($value)) {
          if ($value == array()) {
            $value = 'NULL';
          }
          else {
            // Build a array of values string
            $i=0;
            $val = '';
            foreach ($value as $v) {
              $i++;
              if ($i>1) {
                $val .= ',';
              }
              $val .=   $db->quote($v);
            }
            $value = $val;
          }
        }
        else {
          $value = trim($value);
          $value =    $db->quote($value) ;
        }
      }
    }
    return $value;
  }

}













