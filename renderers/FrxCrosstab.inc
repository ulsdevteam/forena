<?php
class FrxCrossTab extends FrxRenderer {

  private $headers = array();
  private $dim_columns = array();
  private $group_columns = array();
  private $dim_headers = array();
  private $group_headers = array();

  /**
   * Render the crosstab
   */
  public function render() {
    $variables = $this->mergedAttributes();
    $path = isset($variables['path']) ? $variables['path'] : '*';
    if (!$path) $path = "*";


    $group = $variables['group'];
    $dim = $variables['dim'];
    // Get the current context
    $data = Frx::Data()->currentContext();

    // Generate the data nodes.
    if (is_object($data)) {
      if (method_exists($data, 'xpath')) {
        $nodes = $data->xpath($path);
      }
      else {
        $nodes = $data;
      }
    }
    else {
      $nodes = (array)$data;
    }

    // Group the data.
    $data = $this->frxReport->group($nodes, $group);
    $dim_headers = array();
    $dim_rows = array();
    $dim_values = array();
    $rows = array();
    foreach ($data as $gk => $group_rows) {
      $row_copy = array_values($group_rows);
      $dims = $this->frxReport->group($group_rows, $dim);
      $rows[$gk] = $group_rows[0];
      foreach($dims as $dk=>$r) {
        $dims = array_values($r);
        $dim_values[$dk] = $dk;
        $dim_rows[$gk][$dk] = $r[0];
      }
    }

    // Default controling attributes
    $this->defaultHeaders($dim_values);
    $hrow = array();
    foreach ($this->group_headers as $col) {
      $cell = $col;
      if (count($this->dim_columns) > 1) $cell['rowspan'] = 2;
      $hrow[] = $cell;
    }

    // Add the dimension headers.
    foreach ($dim_values as $dk) {
      foreach ($this->dim_headers as $i => $col) {
        $cell = $col;
        $cell['data'] = $dk;
        if (count($this->dim_columns) > 1) {
          $cell['data'] = $i ? $col['data'] : $dk . ' ' . $col['data'];
        }
        $hrow []  = $cell;
      }
    }

    $trows = array();
    foreach ($rows as $k=>$row) {
      Frx::Data()->push($row, '_group');
      $trow = array();
      // Base group
      foreach($this->group_columns as $col) {
        $cell = $col;
        $cell['data'] = $this->teng->replace($col['data']);
        $trow[] = $cell;
      }
      Frx::Data()->pop();

      // Dimensions
      $dim_data = $dim_rows[$k];
      foreach($dim_values as $dk) {
        $dim_row = isset($dim_data[$dk]) ? $dim_data[$dk] : array();
        frx::Data()->push($dim_row, '_dim');
        foreach($this->dim_columns as $col) {
          $cell = $col;
          $cell['data'] = $this->teng->replace($col['data']);
          $trow[] = $cell;
        }
        frx::Data()->pop();
      }
      $trows[] = $trow;

    }
    $vars = array(
    	'header' => $hrow,
      'rows' => $trows,
    );

    $output = theme('table', $vars);
    return $output;
  }

  /**
   * Generate default headers from Embedded xml.
   */
  private function defaultHeaders() {
    $node = $this->reportDocNode;
    if ($node->thead && $node->thead->tr) {
    		foreach ($node->thead->tr->children() as $name => $cell) {
    		  $hcol['data'] = (string)$cell;
    		  $hcol['depth'] = 1;
    		  $hcol['colspan'] = @(string)$cell['colspan'];
    		  $hcol['class'] = @(string)$cell['class'];
    		  $hcol['style'] = @(string)$cell['style'];
    		  if ($name == 'th') {
    		    $this->group_headers[] = $hcol;
    		  }
    		  else {
    		    $this->dim_headers[] = $hcol;
    		  }
    		}
    }
    if ($node->tbody && $node->tbody->tr) {
    		foreach ($node->tbody->tr->children() as $name => $cell) {
    		  $col['data'] = @(string)$cell;
    		  $col['class'] = @(string)$cell['class'];
    		  $col['style'] = @(string)$col['style'];
    		  if ($name == 'th') {
    		    $this->group_columns[] = $col;
    		  }
    		  else {
    		    $this->dim_columns[] = $col;
    		  }
    		}
    }
  }

  /**
   * Crosstab configuration form.
   */
  public function configForm($config) {
    // Load header informationi from parent config.
    $form = parent::configForm($config);
    $this->weight_sort($config['columns']);
    $types = array('heading' => t('Heading'), 'crosstab' => t('Crosstab'), 'value' => 'Value', 'ignore' => t('Ignore'));
    $form['columns'] = array('#theme' => 'forena_element_draggable',   '#draggable_id' => 'FrxTable-columns');
    foreach ($config['columns'] as $key => $col) {
      $ctl = array();
      $ctl['label'] = array(
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Label'),
          '#default_value' => $col['label'],
      );

      $ctl['contents'] = array(
          '#type' => 'textfield',
          '#size' => '30',
          '#title' => t('Data'),
          '#default_value' => $col['contents'],
      );

      $ctl['type'] = array(
          '#type' => 'radios',
          '#title' => t('Type'),
          '#default_value' => $col['type'],
          '#options' => $types,
          '#ajax' => $this->configAjax()
      );

      $ctl['weight'] = array(
          "#type" => 'weight',
          '#title' => t('Weight'),
          '#default_value' => $col['weight'],
      );

      $form['columns'][$key] = $ctl;
    }
    return $form;
  }

  public function generate($xml, &$config) {
    $config['class'] = get_class($this);
    $block = @$config['block'];
    $id = @$config['id'];
    if ($block) {
      $id = $this->idFromBlock($block);
      $config['id'] = $id . '_block';
    }
    $config['class'] = @$config['class'] ? $config['class'] . ' FrxCrosstab' : 'FrxCrosstab';
    $div = $this->blockDiv($config);

    // PUt on the header
    $this->removeChildren($div);
    if (isset($config['header']['value'])) $this->addFragment($div, $config['header']['value']);

    // Decide to inlcude columns
    $found_columns = $this->columns($xml);
    if (!$found_columns) {
      $found_columns = $this->columns($xml, '/*');
      $attrs = array();
    }
    $include_column = 0;
    if (!@$config['columns']) {
      $include_column = 1;
    }

    $numeric_columns = $this->numeric_columns;
    $new_columns = @$config['columns'] ? FALSE : TRUE;

    foreach ($found_columns as $column) {
      $token = '{' . $column . '}';
      if ($new_columns) {
        $type = array_search($column, $this->numeric_columns)!==FALSE ? 'header' : 'value';
      } else {
        $type = 'ignore';
      }
      if (!isset($config['columns'][$column])) {
        $config['columns'][$column] = array(
            'contents' => $token,
            'type' => $type,
            'label' => str_replace('_', ' ', $column),
            'weight' => 1,
        );
      }
    }

    // Generate the grouping row
    $group = '';
    foreach($config['columns'] as $col) {
      if ($col['type'] == 'heading') $group .= $col['contents'];
    }
    $r_id = $id . '-renderer';
    $frx['renderer'] = 'FrxCrosstab';
    $attrs[$id] = $r_id;
    $attrs = array('foreach' => '*');
    $table = $this->setFirstNode($div, 4, 'table');
    $thead = $this->setFirstNode($table, 6, 'thead');
    $throw = $this->setFirstNode($thead, 8, 'tr');
    $tbody = $this->setFirstNode($table, 6, 'tbody');
    $tdrow = $this->setFirstNode($tbody, 8, 'tr', NULL, array('id' => $id),$attrs);
    if ($config['columns']) foreach ($config['columns'] as $key => $col) if ($col['type']!=='ignore') {
      if ($col['type']=='heading') {
        $tag = 'th';
      }
      else {
        $tag = 'td';
      }
      $this->addNode($throw, 10, $tag, $col['label']);
      $this->addNode($tdrow, 10, $tag, $col['contents']);
    }

    if (isset($config['footer']['value'])) $this->addFragment($div, $config['footer']['value']);
  }


}