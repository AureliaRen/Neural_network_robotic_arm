
from sparkai.llm.llm import ChatSparkLLM, ChunkPrintHandler
from sparkai.core.messages import ChatMessage
import re


def nlp_translator(text,is_gripper_command=False):
    SPARKAI_URL = 'wss://spark-api.xf-yun.com/v3.5/chat'
    SPARKAI_APP_ID = 'c6fa08a8'
    SPARKAI_API_SECRET = 'ZjZmYmE3NTQ4ZTNhNzhiODQxNDFiZTYw'
    SPARKAI_API_KEY = '926b859d8a984e4768f3815308631dfd'
    SPARKAI_DOMAIN = 'generalv3.5'
    spark = ChatSparkLLM(
        spark_api_url=SPARKAI_URL,
        spark_app_id=SPARKAI_APP_ID,
        spark_api_key=SPARKAI_API_KEY,
        spark_api_secret=SPARKAI_API_SECRET,
        spark_llm_domain=SPARKAI_DOMAIN,
        streaming=False,
    )
    if not is_gripper_command:
        messages1 = [ChatMessage(
            role="user",
            content=f"请对后面我输入的自然语言进行理解，告诉我是左臂还是右臂发生移动，移动方向是什么，移动距离是多少，把结果整理为一个长度为3的python列表，第一项是左臂或右臂，第二项是移动方向左或右或上或下或前或后，第三项是移动距离数字单位为米。请只输出python列表，不要输出任何多余内容。语言如下：'{text}'"
        )]
    else:
        messages1 = [ChatMessage(
            role="user",
            content=f"请对后面我输入的自然语言进行理解，告诉我是左臂还是右臂的夹爪，夹爪是张开还是关闭，把结果整理为一个长度为2的python列表，第一项是左臂或右臂，第二项是张开或是关闭。请只输出python列表，不要输出任何多余内容。语言如下：'{text}'"
        )]

    handler = ChunkPrintHandler()
    reply = spark.generate([messages1], callbacks=[handler])
    chat_list = reply.generations[0]
    command_list = chat_list[0].text
    #正则表达式提取
    pattern = r'\[([^\]]+)\]' # 使用正则表达式匹配中括号内的内容
    match = re.search(pattern, command_list)
    if match:
        # 按逗号分割成列表
        result = match.group(1).split(', ')
        return result
    else:
        print("未匹配到有效的内容")
        return None






