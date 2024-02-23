/*
 * 定义LTL Formula结构
 * File:   ltl_formula.h
 * Author: yaoyinbo
 *
 * Created on October 20, 2013, 8:22 PM
 */

#ifndef LTL_FORMULA_H
#define	LTL_FORMULA_H

#ifdef	__cplusplus
extern "C"
{
#endif

  /**
   * 操作符类型
   */
  typedef enum _OperationType
  {
    eUNDEFINED = 0,
    eTRUE,
    eFALSE,
    eLITERAL,
    eNOT,
    eNEXT,
    eWNEXT,
    eGLOBALLY,
    eFUTURE,
    eUNTIL,
    eWUNTIL,
    eRELEASE,
    eSINCE,
    eONCE,
    eHISTORICALLY,
    eTRIGGER,
    eYESTERDAY,
    eZYESTERDAY,
    eAND,
    eOR,
    eIMPLIES,
    eEQUIV
  } EOperationType;

  /**
   * 表达式树结构定义
   */
  typedef struct _ltl_formula
  {
    EOperationType _type;
    struct _ltl_formula *_left;
    struct _ltl_formula *_right;

    char *_var;
  } ltl_formula;

  typedef struct _ltl_formulas {
    unsigned int size;
    unsigned int maxsize;
    ltl_formula ** formulas;
    ltl_formula ** names;
  } ltl_formulas;

  /**
   * 构建变量表达式
   * @param var 变量名
   * @return
   */
  ltl_formula *create_var (const char *var);

  /**
   * 构建操作表达式
   * @param type
   * @param left
   * @param right
   * @return
   */
  ltl_formula *create_operation (EOperationType type, ltl_formula *left, ltl_formula *right);

  /**
   * 打印以root为根的表达式树
   * @param root
   */
  void print_formula (ltl_formula *root);

  /**
   * 销毁以root为根的表达式树，
   * 生成后必须显示销毁
   * @param root
   */
  void destroy_formula (ltl_formula *root);

  /**
   * 销毁节点node，不会递归销毁子节点
   * @param node
   */
  void destroy_node (ltl_formula *node);

  ltl_formulas * allocate_ltl_formulas(unsigned int init);
  void free_ltl_formulas(ltl_formulas * self);
  void push_ltlformula(ltl_formulas * self,
		       ltl_formula * name, ltl_formula * formula);
#ifdef	__cplusplus
}
#endif

#endif	/* LTL_FORMULA_H */
