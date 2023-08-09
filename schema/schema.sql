create table `file`(
  id integer primary key,
  path varchar(1024),
  constraint uk_file unique(path)
);

create table decl(
  id integer primary key,
  type varchar(60),
  name varchar(1024),
  file_id int,
  start_line int,
  end_line int,
  brief_comment text,
  comment text,

  -- typedef and using
  underlying_type varchar(100),

  is_struct bool,
  is_abstract bool,
  is_template bool,
  is_scoped bool,

  constraint uk_decl unique(name),
  constraint fk_decl_file foreign key (file_id) references `file`(id) on delete cascade
);

create table template_parameter(
  id integer primary key,
  template_id int,
  template_type varchar(20) not null,
  kind varchar(30),
  type varchar(100),
  name varchar(100),
  value varchar(200),
  is_variadic bool,
  `index` int, 
  constraint uk_template_parameter unique(template_type, template_id, name),
  constraint fk_template_parameter_template foreign key (template_id) references decl(id) on delete cascade
);

create table decl_base(
  id integer primary key,
  decl_id int,
  base_id int,
  position int,
  access varchar(30),
  constraint uk_decl_base unique(decl_id, base_id),
  constraint fk_decl_base_decl foreign key (decl_id) references decl(id) on delete cascade,
  constraint fk_decl_base_base foreign key (decl_id) references decl(id) on delete cascade
);
create table `type`(
  id integer primary key,
  name varchar(200) not null,
  decl_name varchar(200) not null,
  decl_kind varchar(30),
  indirection varchar(20),
  template_parameter_index int,
  constraint uk_type unique(name, template_parameter_index)
);
create table `type_argument`(
  id integer primary key,
  type_id int,
  kind varchar(32),
  `value` varchar(200),
  `index` int,
  constraint fk_type_argument_template foreign key (type_id) references `type`(id) on delete cascade,
  constraint uk_type_argument_template_index unique(type_id, `index`)
);
create table decl_field(
  id integer primary key,
  decl_id int,
  type_id int,
  name varchar(100),
  access varchar(30),
  brief_comment text,
  comment text,
  file_id int,
  start_line int,
  end_line int,
  constraint uk_decl_field unique(decl_id, name),
  constraint fk_decl_field_decl foreign key (decl_id) references decl(id) on delete cascade,
  constraint fk_decl_field_type foreign key (type_id) references `type`(id) on delete cascade
);
create table enum_field(
  id integer primary key,
  enum_id int,
  name varchar(100),
  value int,
  brief_comment text,
  comment text,
  file_id int,
  start_line int,
  end_line int,
  constraint uk_enum_field unique(enum_id, name)
);

create table func(
  id integer primary key,
  name varchar(200),
  qual_name varchar(200),
  signature varchar(512),
  file_id int,
  start_line int,
  end_line int,
  brief_comment text,
  comment text,
  decl_id int,
  type_id int,
  access varchar(32),
  is_static bool,
  is_inline bool,
  is_virtual bool,
  is_pure bool,
  is_ctor bool,
  is_overriding bool,
  is_const bool,
  constraint uk_func unique(signature),
  constraint fk_func_file foreign key (file_id) references file(id) on delete cascade,
  constraint fk_func_type foreign key (type_id) references `type`(id) on delete cascade,
  constraint fk_func_class foreign key (decl_id) references decl(id) on delete cascade
);
create table func_param(
  id integer primary key,
  func_id int,
  position int,
  type_id int,
  name varchar(200),
  default_value varchar(200),
  constraint uk_func_param unique(func_id, position),
  constraint fk_func_param_type foreign key (type_id) references `type`(id) on delete cascade,
  constraint fk_func_param_func foreign key (func_id) references func(id) on delete cascade
);
create table method_override(
  id integer primary key,
  method_id int,
  overridden_method_id int,
  constraint uk_method_override unique(method_id, overridden_method_id),
  constraint fk_method_override_method foreign key (method_id) references func(id) on delete cascade,
  constraint fk_method_override_overridden_method foreign key (overridden_method_id) references func(id) on delete cascade
);
