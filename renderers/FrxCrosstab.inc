<?php
class FrxCrossTab extends FrxRenderer {

  private $headers = array();
  private $columns = array();
  private $dim_columns = array();
  private $group_columns = array();
  private $dim_headers = array();
  private $group_headers = array();


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
    dpm($dim_rows, 'Dimensions');
    dpm($rows, 'Rows');
    dpm($dim_values, 'values');

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
      foreach ($this->dim_headers as $col) {
        $cell = $col;
        $cell['data'] = $dk;
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
    dpm($vars);
    $output = theme('table', $vars);
    return $output;
  }

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
    		  $col['style'] = @(string)['style'];
    		  if ($name == 'th') {
    		    $this->group_columns[] = $col;
    		  }
    		  else {
    		    $this->dim_columns[] = $col;
    		  }
    		}
    }
  }
}