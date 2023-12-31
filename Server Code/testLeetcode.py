import leetcode
# leetcode api from : https://github.com/fspv/python-leetcode
import sys
import json
from time import sleep
from bs4 import BeautifulSoup

def remove_html_tags_and_format(input_html):
    # Parse the HTML string
    soup = BeautifulSoup(input_html, 'html.parser')

    # Get the text content without HTML tags
    text_content = soup.get_text()

    # Replace multiple consecutive whitespaces with a single space
    formatted_text = ' '.join(text_content.split())

    return formatted_text

def handleRequesy(args):
    leetcode_session = ""
    csrf_token = ""
    configuration = leetcode.Configuration()
    configuration.api_key["x-csrftoken"] = csrf_token
    configuration.api_key["csrftoken"] = csrf_token
    configuration.api_key["LEETCODE_SESSION"] = leetcode_session
    configuration.api_key["Referer"] = "https://leetcode.com"
    configuration.debug = False
    api_instance = leetcode.DefaultApi(leetcode.ApiClient(configuration))
    langs = [
      'cpp',
      'java',
      'python',
      'python3',
      'c',
      'csharp',
      'javascript',
      'typescript',
      'php',
      'swift',
      'kotlin',
      'dart',
      'golang',
      'ruby',
      'scala',
      'rust',
      'racket',
      'erlang',
      'elixir'
    ]
    if len(args)>1:
      if args[1] == "difficulty":
        return str(json.dumps({
          "list": [{'easy': 1, 'medium': 2, 'hard': 3}]
        }))
      elif args[1] == "open":
        graphql_request = leetcode.GraphqlQuery(
        query="""
            query getQuestionDetail($titleSlug: String!) {
              question(titleSlug: $titleSlug) {
                questionId
                questionFrontendId
                boundTopicId
                title
                content
                translatedTitle
                isPaidOnly
                difficulty
                likes
                dislikes
                isLiked
                similarQuestions
                contributors {
                  username
                  profileUrl
                  avatarUrl
                  __typename
                }
                langToValidPlayground
                topicTags {
                  name
                  slug
                  translatedName
                  __typename
                }
                companyTagStats
                codeSnippets {
                  lang
                  langSlug
                  code
                  __typename
                }
                stats
                codeDefinition
                hints
                solution {
                  id
                  canSeeDetail
                  __typename
                }
                status
                sampleTestCase
                enableRunCode
                metaData
                translatedContent
                judgerAvailable
                judgeType
                mysqlSchemas
                enableTestMode
                envInfo
                __typename
              }
            }
        """,
        variables=leetcode.GraphqlQueryVariables(title_slug=args[2]),
        operation_name="getQuestionDetail",
    )

    # api_response = api_instance.graphql_post(body=graphql_request)
        apiRequest = api_instance.graphql_post(body=graphql_request)
        questionData = apiRequest.data.question
        # print(questionData.sample_test_case)
        listLang = questionData.code_snippets
        codeStr = ""
        lang_slug = "python"
        if len(args)>3:
          lang_slug = args[3]
        for i in listLang:
          if i.lang_slug == lang_slug:
            codeStr = i.code
            break
        return json.dumps({
          'title': args[2],
          'test_case': questionData.sample_test_case,
          'detail': str(remove_html_tags_and_format(questionData.content)),
          'code': codeStr
        })
        # return str(remove_html_tags_and_format(api_instance.graphql_post(body=graphql_request).data.question.content))
        # print(args)
      elif args[1] == "submit":
        massage = "success"
        title = ""
        codeLang = ""
        codeValue = ""
        #check login
        if(len(args)>1):
          title = args[2]
        if(len(args)>2):
          codeLang = args[3]
        if(len(args)>3):
          codeValue= args[4]
        code = """
class Solution(object):
  def twoSum(self, nums, target):
    cache = {}
    for idx, n in enumerate(nums):
      cv = cache.get(target - n)
      if cv != None and cv != idx:
        return [cv, idx]
      cache[n] = idx
    return []
"""
        test_submission = leetcode.TestSubmission(
            data_input="",
            typed_code=code,
            question_id=2,
            test_mode=False,
            lang="python",
        )
        interpretation_id = api_instance.problems_problem_interpret_solution_post(
            problem="two-sum", body=test_submission
        )
        sleep(5)  # FIXME: should probably be a busy-waiting loop
        test_submission_result = api_instance.submissions_detail_id_check_get(
            id=interpretation_id.interpret_id
        )
        
        print("Got test result:")
        print(test_submission_result)
        # print(leetcode.TestSubmissionResult(**test_submission_result))
        return str(json.dumps({'massage': massage}))
      elif args[1] == "langs":
        return str(json.dumps({'list': langs}))
      elif args[1] == "category":
        listCategory = ['algorithms','Database', 'concurrency', 'javascript', 'pandas']
        return str(json.dumps({'list':listCategory}))
      elif args[1] == "difficulties":
        if(len(args)<2): return str(json.dumps({"massage": "parameter not enoughh"}))
        api_response=api_instance.api_problems_topic_get(topic="all")
        solved_questions=[]
        for questions in api_response.stat_status_pairs:
          if(questions.difficulty.level==int(args[2])):
            solved_questions.append(questions.to_dict())
        return str(json.dumps({'list': solved_questions}))
      elif args[1] == "list":
        if(len(args)<2): return str(json.dumps({"massage": "parameter not enoughh"}))
        api_response=api_instance.api_problems_topic_get(topic=args[2])
        solved_questions=[]
        for questions in api_response.stat_status_pairs:
          solved_questions.append(questions.to_dict())
        return str(json.dumps({'list': solved_questions}))

# print(leetcode)
# if __name__ == "__main__":
#   print(handleRequesy(sys.argv))
# graphql_request = leetcode.GraphqlQuery(
# query="""
#      {
#        user {
#             username
#             isCurrentUserPremium
#          }
#      }
#      """,
# variables=leetcode.GraphqlQueryVariables(),
# )
# print(api_instance.graphql_post(body=graphql_request))
# api_response=api_instance.api_problems_topic_get(topic="algorithms")
# solved_questions=[]
# for questions in api_response.stat_status_pairs:
#     if questions.status=="ac":
#        print(questions.stat)
#create list problem
#create list category problem
#create list dificulty
#create set session to leetcode api with sql database
#create window category to save, some will save to leetode api, or 
#add save category in window ps vita app

# examlple command:
# get all category : leetcode category
# response ['algorithm', 'data structure', 'database']
# get programming list: leetcode langs
# response ['python', 'c++']
# list problem in category : leetcode list algorithm
# response ['two-factor', 'salesman']
# open problems : leetcode open two-factor (withh default lang=python) or leetcode open two-factor lang=pythhon
# request if login then return data submitted etch and if not login just return response
# response { 'title':'two-factor', 'descripton': 'vafa',  'code': 'def lol: return "hello"'}
# open editor window and return desc
# submit code : leetcode submit two-factor python (code input base64)
# request has to login

# leetcode 