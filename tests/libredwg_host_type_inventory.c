/* Ephemeral GitHub Actions reference executable, NOT part of MusaCAD.
 * Reads the same public DWG through pinned upstream LibreDWG and emits ONLY
 * a minimal, typed OBJECTS list. No geometry, handles, layer names, text,
 * source coordinates or third-party source are included.
 * This program links to LibreDWG exclusively in the isolated CI host.
 */
#include <dwg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int
safe_type_name(const char *name)
{
  size_t i;
  if (!name || !name[0])
    return 0;
  for (i = 0; i < 64 && name[i]; i++)
    {
      const unsigned char c = (unsigned char)name[i];
      if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
            || c == '_'))
        return 0;
    }
  return i > 0 && i < 64 && name[i] == '\0';
}

int
main(int argc, char **argv)
{
  Dwg_Data dwg = { 0 };
  int error;
  size_t i, count;
  if (argc != 2)
    return 2;

  error = dwg_read_file(argv[1], &dwg);
  if (error >= DWG_ERR_CRITICAL)
    {
      dwg_free(&dwg);
      return 3;
    }
  if (!dwg.object || !dwg.num_objects || dwg.num_objects > 2000000U)
    {
      dwg_free(&dwg);
      return 4;
    }
  count = (size_t)dwg.num_objects;
  printf("{\"DECODER_WARNING_MASK\":%d,\"OBJECTS\":[", error);
  for (i = 0; i < count; i++)
    {
      const Dwg_Object *obj = &dwg.object[i];
      const char *kind = NULL;
      if (obj->supertype == DWG_SUPERTYPE_ENTITY)
        kind = "entity";
      else if (obj->supertype == DWG_SUPERTYPE_OBJECT)
        kind = "object";
      if (i)
        putchar(',');
      if (kind && safe_type_name(obj->name))
        printf("{\"%s\":\"%s\"}", kind, obj->name);
      else
        fputs("{}", stdout);
    }
  fputs("]}\n", stdout);
  error = ferror(stdout) ? 5 : 0;
  dwg_free(&dwg);
  return error;
}
