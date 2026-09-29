from sparkai.llm.llm import ChatSparkLLM, ChunkPrintHandler
from sparkai.core.messages import ChatMessage

#星火认知大模型Spark Max的URL值，其他版本大模型URL值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_URL = 'wss://spark-api.xf-yun.com/v3.5/chat'
#星火认知大模型调用秘钥信息，请前往讯飞开放平台控制台（https://console.xfyun.cn/services/bm35）查看
SPARKAI_APP_ID = 'c6fa08a8'
SPARKAI_API_SECRET = 'ZjZmYmE3NTQ4ZTNhNzhiODQxNDFiZTYw'
SPARKAI_API_KEY = '926b859d8a984e4768f3815308631dfd'
#星火认知大模型Spark Max的domain值，其他版本大模型domain值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_DOMAIN = 'generalv3.5'

if __name__ == '__main__':
    spark = ChatSparkLLM(
        spark_api_url=SPARKAI_URL,
        spark_app_id=SPARKAI_APP_ID,
        spark_api_key=SPARKAI_API_KEY,
        spark_api_secret=SPARKAI_API_SECRET,
        spark_llm_domain=SPARKAI_DOMAIN,
        streaming=False,
    )
    messages1 = [ChatMessage(
        role="user",
        content='请对后面我输入的自然语言进行理解，告诉我是左臂还是右臂发生移动，移动方向是什么，移动距离是多少，把结果整理为一个长度为2的python列表，第一项是移动方向左或右，第二项是移动距离数字单位为米。请只输出python列表，不要输出任何多余内容。语言如下：手臂向左移动15毫米'
    )]
    handler = ChunkPrintHandler()
    a = spark.generate([messages1], callbacks=[handler])
    b = a.generations[0]
    c = b[0].text
    print(c)
'''"```python
['左', 0.015]
```"'''
