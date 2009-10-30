<?php
// $Id$

/**
 * Class that defines the picture resource
 *
 * @TargetedAction(name='file', controller='uploadFile')
 */
class DocuWalkPictureResource {
  /**
   * Creates a picture
   *
   * @param object $picture ["data"]
   * @return object
   *
   * @Access(callback='DocuWalkPictureResource::access', args={'create'}, appendArgs=false)
   */
  public static function create($picture) {
    global $user;
    $attr = array('solution', 'position');
    $node = (object)array(
      'type' => 'docuwalk_picture',
      'created' => time(),
      'modified' => time(),
      'uid' => $user->uid,
    );

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($picture->$name)) {
        $node->$name = $picture->$name;
      }
      else {
        return services_error("Missing attribute {$name}", 406);
      }
    }

    $node->title = drupal_substr($node->body, 0, min(drupal_strlen($node->body), 40));

    // Format the solution nid for cck
    $node->field_docuwalk_solution = array(
      array('nid' => $node->solution),
    );
    unset($node->solution);

    $node->simple_geo_position = $node->position;
    unset($node->position);

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-picture', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Retrieves a picture
   *
   * @param int $nid ["path","0"]
   *  The nid of the picture to get
   * @return object
   *
   * @Access(callback='DocuWalkPictureResource::access', args={'view'}, appendArgs=true)
   */
  public static function retrieve($nid) {
    return node_load($nid);
  }

  /**
   * Updates a picture
   *
   * @param int $nid ["path","0"]
   *  The nid of the picture to update
   * @param object $picture ["data"]
   *  The picture object
   * @return object
   *
   * @Access(callback='DocuWalkPictureResource::access', args={'update'}, appendArgs=true)
   */
  public static function update($nid, $picture) {
    $attr = array('body');
    $node = node_load($nid);

    // Transfer attributes from
    foreach ($attr as $name) {
      if (isset($picture->$name)) {
        $node->$name = $picture->$name;
      }
    }

    $picture->title = drupal_substr($picture->body, 0, min(drupal_strlen($picture->body), 40));

    node_save($node);

    return (object)array(
      'nid' => $node->nid,
      'uri' => services_resource_uri(array('docuwalk-picture', $node->nid)),
      'url' => url('node/' . $node->nid, array('absolute' => TRUE))
    );
  }

  /**
   * Deletes a picture
   *
   * @param int $nid ["path","0"]
   *  The nid of the picture to delete
   * @return bool
   *
   * @Access(callback='DocuWalkPictureResource::access', args={'delete'}, appendArgs=true)
   */
  public static function delete($nid) {
    node_delete($nid);
  }

  /**
   * Retrieves a listing of picturess
   *
   * @param int $page ["param","page"]
   * @param string $fields ["param","fields"]
   * @param array $parameters ["param"]
   * @return array
   *
   * @Access(callback='user_access', args={'access content'}, appendArgs=false)
   */
  public static function index($page=0, $fields=array(), $parameters=array()) {
    $builder = new QueryBuilder();
    if ($params['__action']=='describe') {
      return $builder->describe();
    }

    if (is_string($fields)) {
      $fields = preg_split('/,\s?/', $fields);
    }
    $parameters['status'] = 1;
    $parameters['type'] = 'docuwalk_picture';

    list($sql, $params) = $builder->query($fields, $parameters);
    $res = db_query_range($sql, $params, $page*20, 20);

    $nodes = array();
    while ($node = db_fetch_object($res)) {
      $node->uri = services_resource_uri(array('docuwalk-picture', $node->nid));
      $nodes[] = $node;
    }
    return $nodes;
  }

  /**
   * Attaches a file to the image node
   *
   * @param int $nid ["path","0"]
   *  The nid of the image to update
   * @param object $file ["data"]
   *  The file object
   * @return object
   *
   * @Access(callback='DocuWalkPictureResource::access', args={'update'}, appendArgs=true)
   * @RESTRequestParser(mime='image/*', parser='RESTServer::fileRecieve')
   */
  public static function uploadFile($nid, $file) {
    global $user;
    $node = node_load($nid);

    // Mark old file as temporary, then it'll be removed on next cron run, and we don't have to bother
    // ourselves with the business of deleting the file and the database entry for it
    if (!empty($node->field_picture) && $node->field_picture[0]['fid']) {
      $old_file = (object)$node->field_picture[0];
      file_set_status($old_file, FILE_STATUS_TEMPORARY);
    }
    

    $file->uid = $user->uid;
    $file->status = FILE_STATUS_PERMANENT;
    $destination = file_destination(file_create_path(
      file_directory_path() . '/pictures/' . $file->filename));
    rename($file->filepath, $destination);
    $file->filepath = $destination;

    drupal_write_record('files', $file, array('fid'));

    $node->field_picture = array((array)$file);
    node_save($node);

    return $file;
  }

  /**
   * Access checking function.
   *
   * @param string $op
   *  The operation that the user wants to perform.
   * @param array $args
   *  The arguments for the call.
   * @return bool
   *  True if access is given, otherwise false.
   */
  public static function access($op, $args) {
    if ($op=='create') {
      $node = (object)array(
        'type' => 'docuwalk_picture',
      );
    }
    else if (!empty($args)) {
      $node = node_load($args[0]);
    }
    return node_access($op, $node);
  }
}