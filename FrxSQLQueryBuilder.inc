<?php
/**
 * FrxSQLQuery SQL Builder
 * This class defines a common query builder that is used to
 * make SQL safe queries based on named column filteres.
 * @author metzlerd
 */
class FrxSQLQueryBuilder {
	private $clauses = array();
	private $descriptions = array();
	private $applied_filters = array();
	private $where_clause = '';
	private $comparison_operators= array(
	  '<' => 'is less than',
	  '>' => 'is greater than',
	  '=' => 'is equal to',
	  '<>' => 'is not equal to',
	  'LIKE' => 'is like',
	);
	private $data;
	/**
	 * Add a filter object
	 * Enter description here ...
	 * @param unknown_type $field
	 * @param unknown_type $comparison
	 * @param unknown_type $sql
	 * @param unknown_type $title
	 */
	public function defineFilter($field, $comparison, $sql, $title='') {
		// Intialise the field if not set
		if (!isset($this->clauses[$field])) {
			$this->clauses[$field] = array();
		}

		// Add the where clause from the lookup
		$this->clauses[$field][$comparison] = $sql;;
		// Add the title
		$title = $title ? $title : $field . ' is ' . $comparison;
		$this->descriptions[$field][$comparison] = $title;
		return $this;
	}


  /**
   * Set up common column filters
   * This will set up all of the common filters on data ...
   * @param unknown_type $columns
   * @param unknown_type $comparisons
   */
	public function defineColumnFilters($columns, $comparisons=array()) {
		$columns=(array)$columns;
		if ($columns) foreach ($columns as $column ) {
      if (!$comparisons) $comparisons = array_keys($this->comparison_operators);
			if ($comparisons) foreach ($comparisons as $comparison) {
				if (array_key_exists($comparison,$this->comparison_operators)) {
				  $clause = $column . ' ' . $comparison . ' :'. $column;
		      $this->defineFilter($column, $comparison, $clause, $this->comparison_operators[$comparison]);
			  }
			}

		}
		return $this;
	}

	/**
	 * Helper functon to build common numeric filters.
	 * Enter description here ...
	 * @param unknown_type $columns
	 */
	public function defineNumericFilters($columns) {
		$this->defineColumnFilters($columns, array('<','>','=','<>'));
		return $this;
	}

	/**
	 * Starts the query builder
	 * Enter description here ...
	 */
	public function query($block, $values = array()) {
		$this->applied_filters = array();
		$this->block=$block;
		$this->data = $values;
		drupal_set_message(print_r($values,1));
		return $this;
	}

	/**
	 * Executes the query that was started with query method
	 * Enter description here ...
	 */
	public function execute() {
		$where='';
		if ($this->where_clause) {
			$where  = 'WHERE 1=1 ' .$this->where_clause;
		}
    return forena_invoke_data_provider($this->block, $this->data, $where);

	}

	/**
	 * Filters the query that is being executeed
	 * Enter description here ...
	 * @param string $field
	 * @param string $comparison
	 */
  public function filter($field, $comparison) {
    drupal_set_message(print_r($this->clauses,1));
  	if (@$this->clauses[$field][$comparison]) {
	  	$this->where_clause .= ' AND ' . $this->clauses[$field][$comparison];
	  	drupal_set_message('in here');
  	}
  	drupal_set_message($this->block);
  	return $this;
  }
}